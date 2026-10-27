/**
  ******************************************************************************
  * @file    dq110_protect.c
  * @brief   保护：过流分级保护 + 堵转检测 + IWDG 看门狗
  *
  *         过流：10ms 采样 + 8 点滑动平均 + 连续 4 次超阈值 → FAULT（响应 ≤100ms）
  *         堵转：运行中超过 DQ110_STALL_MS 无霍尔脉冲 → FAULT
  *         FAULT 由回 0 档清除（见 dq110_power.c 状态机）
  ******************************************************************************
  */
#include "main.h"
#include "dq110_protect.h"
#include "dq110_adc.h"
#include "dq110_tim.h"

IWDG_HandleTypeDef DQ110_IwdgHandle;

static uint32_t s_run_start_ms = 0;
static uint8_t  s_trip_cnt = 0;
static uint8_t  s_lowv_cnt = 0;
static uint32_t s_last_lowv = 0;

/**
  * @brief  LSI + IWDG 初始化（8s 超时，主循环 10ms 喂狗）
  * @note   IWDG 由 LSI 驱动，启动后不可关闭；0 档进 Stop 前必须关 LSI 冻结
  *         IWDG（datasheet：IWDG 在 stop 模式仍计数，会以复位方式唤醒）
  */
void DQ110_IWDG_Init(void)
{
  RCC_OscInitTypeDef rcc = {0};

  rcc.OscillatorType = RCC_OSCILLATORTYPE_LSI;
  rcc.LSIState       = RCC_LSI_ON;
  if (HAL_RCC_OscConfig(&rcc) != HAL_OK) {
    APP_ErrorHandler();
  }

  DQ110_IwdgHandle.Instance       = IWDG;
  DQ110_IwdgHandle.Init.Prescaler = IWDG_PRESCALER_64;   /* LSI 32.768k/64 = 512Hz */
  DQ110_IwdgHandle.Init.Reload    = 4095;                /* 4095/512 = 8.0s */
  if (HAL_IWDG_Init(&DQ110_IwdgHandle) != HAL_OK) {
    APP_ErrorHandler();
  }
}

/**
  * @brief  进入 RUN 时记录启动时刻（堵转检测宽限期起点）
  */
void DQ110_Protect_StartRun(uint32_t now_ms)
{
  s_run_start_ms = now_ms;
  s_trip_cnt = 0;
  s_lowv_cnt = 0;
}

/**
  * @brief  保护主循环（10ms 节拍）：
  *         过流分级检测 + 堵转检测，触发后进入 FAULT（停 PWM）
  */
void DQ110_Protect_Run(uint32_t now_ms)
{
  static uint32_t s_last_samp = 0;
  uint16_t raw;
  uint32_t thr;

  if ((now_ms - s_last_samp) < 10U) return;
  s_last_samp = now_ms;

  if (g_dq110_state != DQ110_STATE_RUN) {
    s_trip_cnt = 0;
    return;
  }

  /* ---- 过流：按档位分级阈值（两版本通用） ---- */
  raw = DQ110_ADC_ReadFiltered();
  thr = (g_dq110_gear == DQ110_GEAR2) ? DQ110_OC_ThresholdLsb(DQ110_OC_GEAR2_MA)
                                      : DQ110_OC_ThresholdLsb(DQ110_OC_GEAR1_MA);
  if (raw >= thr) {
    if (++s_trip_cnt >= DQ110_OC_TRIP_N) {
      g_dq110_state = DQ110_STATE_FAULT;
      DQ110_TIM_SetDuty(0);
      DQ110_TIM_PWM_Stop();
      return;
    }
  } else {
    s_trip_cnt = 0;
  }

#ifndef DQ110_NO_HALL
  /* ---- 堵转（仅霍尔版）：运行中超过宽限期仍无霍尔脉冲 ---- */
  if ((now_ms - s_run_start_ms) > DQ110_START_GRACE_MS) {
    if ((now_ms - g_dq110_last_cap_ms) > DQ110_STALL_MS) {
      g_dq110_state = DQ110_STATE_FAULT;
      DQ110_TIM_SetDuty(0);
      DQ110_TIM_PWM_Stop();
    }
  }
#else
  /* ---- 低压保护（仅低成本无霍尔版）：电池电压 < 2.0V 关断，防止过放 ----
   * 100ms 采一次 VREFINT 反推电压，连续 2 次确认（200ms） */
  if ((now_ms - s_last_lowv) >= 100U) {
    uint16_t vref = DQ110_ADC_ReadVrefintRaw();
    uint32_t vbat_mv;
    if (vref == 0) return;
    vbat_mv = (uint32_t)DQ110_VREFINT_MV * 4095U / (uint32_t)vref;
    if (vbat_mv < DQ110_VBAT_LOW_MV) {
      if (++s_lowv_cnt >= 2U) {
        g_dq110_state = DQ110_STATE_FAULT;
        DQ110_TIM_SetDuty(0);
        DQ110_TIM_PWM_Stop();
      }
    } else {
      s_lowv_cnt = 0;
    }
    s_last_lowv = now_ms;
  }
#endif
}
