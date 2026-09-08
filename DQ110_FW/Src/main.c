/**
  ******************************************************************************
  * @file    main.c
  * @brief   DQ110 双速直流马达驱动器固件主程序（PY32F002B SOP8 版）
  *
  *         流程：时钟 24MHz → GPIO → TIM14(20kHz 软件 PWM 时基)
  *         主循环 10ms：档位防抖扫描 → 按档设占空比
  *           0 档停 / 一档 40%(15转) / 二档 80%(30转)
  *         马达 PWM 输出 = PB6(pin5)，软件 PWM
  ******************************************************************************
  */
#include "main.h"
#include "dq110_gpio.h"
#include "dq110_tim.h"
#include "dq110_adc.h"

/* 模块间共享全局（dq110.h 声明） */
volatile DQ110_Gear_t g_dq110_gear = DQ110_GEAR_OFF;

/* 慢启动斜坡状态（10ms 节拍） */
static uint16_t s_cur_duty = 0;    /* 当前实际输出占空比 */
static uint16_t s_tgt_duty = 0;    /* 目标占空比（档位决定） */
static uint8_t  s_pwm_on   = 0;    /* TIM14 时基运行标志（0 档停） */
static uint8_t  s_volt_cnt = 0;    /* 100ms 电压采样计数器 */
static uint8_t  s_ocd_cnt  = 0;    /* 过载连续超阈值计数 */
static uint8_t  s_ocd_fault = 0;   /* 过载锁存（拨回 0 档清除） */
static uint32_t s_gear_ms  = 0;    /* 最近拨档时刻（过载启动豁免计时） */
uint16_t DQ110_AD_LIVE;            /* 实时电流 AD（SWD 可读，供调试显示） */

/**
  * @brief  按当前档位 + 电池电压查补偿表，算出目标占空比级数
  */
static uint16_t DQ110_TargetForGear(void)
{
  uint32_t base;
  uint32_t lvl;
  int      idx;
  uint16_t v;

  if (g_dq110_gear == DQ110_GEAR_OFF) {
    return 0;
  }
  base = (g_dq110_gear == DQ110_GEAR1) ? DQ110_G1_HALF_BASE : DQ110_G2_HALF_BASE;

  v   = DQ110_ADC_GetVbatMv();
  /* 表索引：v 越高 idx 越小（idx0=3.2V, K=256; idx12=2.6V, K=315） */
  idx = ((int)DQ110_VBAT_BASE_MV - (int)v) / (int)DQ110_VBAT_STEP_MV;
  if (idx < 0) idx = 0;                                  /* v > 3.2V：按 3.2V 基准 */
  if (idx >= (int)DQ110_VBAT_LEVELS) idx = (int)DQ110_VBAT_LEVELS - 1;

  lvl = (base * (uint32_t)DQ110_VBAT_K[idx] + 128U) / 256U;   /* 四舍五入 */
  if (lvl > DQ110_PWM_MAX) lvl = DQ110_PWM_MAX;
  return (uint16_t)lvl;
}

/**
  * @brief  系统时钟：HSI 24MHz，AHB/APB 不分频
  */
static void DQ110_SystemClock_Config(void)
{
  RCC_OscInitTypeDef ro = {0};
  RCC_ClkInitTypeDef rc = {0};

  ro.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
  ro.HSIState            = RCC_HSI_ON;
  ro.HSICalibrationValue = RCC_HSICALIBRATION_24MHz;   /* 24MHz 校准 */
  if (HAL_RCC_OscConfig(&ro) != HAL_OK) {
    APP_ErrorHandler();
  }

  rc.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
  rc.SYSCLKSource   = RCC_SYSCLKSOURCE_HSISYS;
  rc.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  rc.APB1CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&rc, FLASH_LATENCY_0) != HAL_OK) {
    APP_ErrorHandler();
  }
}

/**
  * @brief  按当前档位设定 PWM 目标占空比 + 启停 PWM 时基
  *         0 档：停 TIM14 中断（省电，输出拉低）；有档：启动 TIM14（幂等）
  */
static void DQ110_ApplyGear(void)
{
  if (g_dq110_gear != DQ110_GEAR_OFF) {
    s_gear_ms = HAL_GetTick();           /* 记录拨档时刻（过载豁免用） */
    if (!s_pwm_on) {
      DQ110_TIM_PWM_Start();         /* 启动 TIM14 中断 */
      s_pwm_on = 1;
    }
    s_tgt_duty = DQ110_TargetForGear();   /* 档位 × 电压补偿表目标 */
  } else {
    if (s_pwm_on) {
      DQ110_TIM_PWM_Stop();          /* 停中断 + PWM 拉低 */
      s_pwm_on = 0;
    }
    s_tgt_duty = 0;
    s_cur_duty = 0;
    DQ110_TIM_SetDuty(0U);
    s_ocd_fault = 0;                 /* 拨回 0 档清除过载锁存 */
    s_ocd_cnt   = 0;
  }
}

/**
  * @brief  过载检测（主循环 50ms 调用）
  *         AD(PA7 电流) > DQ110_OCD_TH_AD 连续 3 次 → 锁存停机；
  *         拨档后 600ms 内豁免（滤启动浪涌）；每点更新 DQ110_AD_LIVE 供调试
  */
static void DQ110_OcdTick(void)
{
  uint16_t raw;

  if (s_ocd_fault || !s_pwm_on) {          /* 已锁存或未运行 */
    s_ocd_cnt = 0;
    return;
  }
  if ((HAL_GetTick() - s_gear_ms) < DQ110_OCD_BLANK_MS) {   /* 启动豁免 */
    s_ocd_cnt = 0;
    return;
  }
  raw = DQ110_ADC_GetCurrentAd();          /* PA7=ADC_IN4 电流 */
  DQ110_AD_LIVE = raw;                     /* 保持实时读 AD（SWD 调试用） */
  if (raw == 0xFFFFU) {
    return;
  }
  if (raw > DQ110_OCD_TH_AD) {
    if (++s_ocd_cnt >= DQ110_OCD_CONFIRM_CNT) {
      s_ocd_fault = 1;                     /* 过载锁存：立即停 PWM */
      s_ocd_cnt   = 0;
      DQ110_TIM_PWM_Stop();
      s_pwm_on    = 0;
      s_tgt_duty  = 0;
      s_cur_duty  = 0;
      DQ110_TIM_SetDuty(0U);
    }
  } else {
    s_ocd_cnt = 0;
  }
}

/**
  * @brief  慢启动斜坡（主循环每 10ms 调用）
  *         停机(目标 0)：立即切断；启动/换挡：每 10ms 逼近 DQ110_RAMP_STEP
  */
static void DQ110_RampTick(void)
{
  if (s_tgt_duty == 0U) {                        /* 0 档：立即停 */
    if (s_cur_duty != 0U) {
      s_cur_duty = 0U;
      DQ110_TIM_SetDuty(0U);
    }
    return;
  }
  if (s_cur_duty < s_tgt_duty) {                 /* 爬升（启动/升档） */
    s_cur_duty += DQ110_RAMP_STEP;
    if (s_cur_duty > s_tgt_duty) s_cur_duty = s_tgt_duty;
    DQ110_TIM_SetDuty(s_cur_duty);
  } else if (s_cur_duty > s_tgt_duty) {          /* 降档斜坡 */
    s_cur_duty -= DQ110_RAMP_STEP;
    if (s_cur_duty < s_tgt_duty) s_cur_duty = s_tgt_duty;
    DQ110_TIM_SetDuty(s_cur_duty);
  }
}

/**
  * @brief  主程序
  */
int main(void)
{
  HAL_Init();
  DQ110_SystemClock_Config();

  DQ110_GPIO_Init();     /* 档位输入 + PWM 输出脚 PB6 + AD 脚 PA7 */
  DQ110_ADC_Init();      /* VREFINT 电压检测 */
  DQ110_TIM_Init();      /* TIM14 软件 PWM 时基（由档位逻辑启停） */

  /* 上电即读一次档位（若上电时已在档位，防抖确认后自动启动） */
  DQ110_Gear_Scan();

  while (1) {
    if (DQ110_Gear_Scan()) {         /* 档位变化（10ms 防抖确认） */
      DQ110_ApplyGear();             /* 设目标：0 档停 / 档位 × 电压补偿 */
    }
    if (++s_volt_cnt >= 10U) {       /* 每 100ms：电压采样 + 过载检测 */
      s_volt_cnt = 0;
      DQ110_OcdTick();               /* 100ms 过载检测 */
      if (DQ110_ADC_Update() && (g_dq110_gear != DQ110_GEAR_OFF)) {
        s_tgt_duty = DQ110_TargetForGear();   /* 电压下降自动加大占空比（恒速） */
      }
    } else if ((s_volt_cnt % 5U) == 0U) {
      DQ110_OcdTick();               /* 50ms 过载检测 */
    }
    DQ110_RampTick();                /* 慢启动斜坡逼近目标（10ms 步进） */
    HAL_Delay(10);
  }
}

/**
  * @brief  错误处理（初始化失败时停在这里，便于调试）
  */
void APP_ErrorHandler(void)
{
  while (1) {
  }
}

/************************ (C) COPYRIGHT Puya *****END OF FILE****/
