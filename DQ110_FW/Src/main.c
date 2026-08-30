/**
  ******************************************************************************
  * @file    main.c
  * @brief   DQ110 双速直流马达驱动器固件主程序
  *
  *         流程：时钟 24MHz → GPIO/EXTI → ADC → TIM1(PWM+捕获) → PWR → IWDG
  *         主循环 10ms：档位防抖扫描 → 过流/堵转保护 → PID 稳速 → 低功耗状态机 → 喂狗
  ******************************************************************************
  */
#include "main.h"
#include "dq110_gpio.h"
#include "dq110_tim.h"
#include "dq110_adc.h"
#include "dq110_speed.h"
#include "dq110_protect.h"
#include "dq110_power.h"

/* 模块间共享全局（dq110.h 声明） */
volatile DQ110_Gear_t   g_dq110_gear        = DQ110_GEAR_OFF;
volatile DQ110_State_t  g_dq110_state       = DQ110_STATE_STANDBY;
volatile uint8_t        g_dq110_gear_event  = 0;
volatile uint16_t       g_dq110_duty        = 0;
volatile float          g_dq110_rpm         = 0.0f;
volatile uint32_t       g_dq110_last_cap_ms = 0;

/**
  * @brief  系统时钟：HSI 24MHz，AHB/APB 不分频（TIM1 时钟 = 24MHz）
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
  * @brief  主程序
  */
int main(void)
{
  HAL_Init();
  DQ110_SystemClock_Config();

  DQ110_GPIO_Init();     /* SW1/SW2 + EXTI */
  DQ110_ADC_Init();      /* 过流检测 */
  DQ110_TIM_Init();      /* PWM + 捕获（初始占空比 0，马达不转） */
  DQ110_PWR_Init();      /* Stop 模式参数 */
  DQ110_IWDG_Init();     /* LSI + IWDG 8s */

  /* 上电即读一次档位（若上电时已在档位，防抖确认后自动启动） */
  DQ110_Gear_Scan();

  while (1) {
    DQ110_Gear_Scan();                 /* 档位防抖（10ms 节拍） */
    DQ110_Protect_Run(HAL_GetTick());  /* 过流 + 堵转保护 */
    DQ110_Speed_Control(HAL_GetTick());/* PID 稳速（100ms 节拍） */
    DQ110_Power_Manage();              /* 状态机 + Stop 待机 */
    HAL_IWDG_Refresh(&DQ110_IwdgHandle);
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
