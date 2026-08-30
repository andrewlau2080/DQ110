/**
  ******************************************************************************
  * @file    dq110_tim.c
  * @brief   TIM1：PWM 输出（M- 驱动）+ 输入捕获（HALL 测速）
  *
  *         时基：PSC=0，ARR=1199 → PWM 20kHz（CNT tick = 24MHz）
  *         测速：TIM1 更新中断把 16 位 CNT 扩展为 32 位 tick（每 50µs +1200），
  *               捕获中断取 tick32+CCR 得捕获时刻；两次捕获差 = 周期（24MHz tick）
  *               → RPM = 60 × 24MHz / (Δt × PPR)。32 位无符号差自动处理回绕
  *               （两次捕获间隔须 < 179s，本应用最大 4s，满足）。
  ******************************************************************************
  */
#include "main.h"
#include "dq110_tim.h"
#include "dq110_gpio.h"

TIM_HandleTypeDef DQ110_TimHandle;
volatile uint32_t DQ110_CapTick32 = 0;

static uint32_t s_last_cap_t   = 0;
static uint32_t s_last_cap_tick = 0;
static volatile uint16_t s_duty = 0;

/**
  * @brief  TIM1 初始化：20kHz PWM（M- 通道）+ HALL 输入捕获
  */
void DQ110_TIM_Init(void)
{
  TIM_OC_InitTypeDef oc = {0};
  TIM_IC_InitTypeDef ic = {0};

  DQ110_TimHandle.Instance               = DQ110_PWM_TIM;
  DQ110_TimHandle.Init.Prescaler         = 0;
  DQ110_TimHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
  DQ110_TimHandle.Init.Period            = DQ110_PWM_PERIOD;      /* 1199 → 20kHz */
  DQ110_TimHandle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  DQ110_TimHandle.Init.RepetitionCounter = 0;
  DQ110_TimHandle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&DQ110_TimHandle) != HAL_OK) {
    APP_ErrorHandler();
  }

  /* PWM 通道：PWM1 模式，高有效，初始占空比 0 */
  oc.OCMode       = TIM_OCMODE_PWM1;
  oc.Pulse        = 0;
  oc.OCPolarity   = TIM_OCPOLARITY_HIGH;
  oc.OCFastMode   = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&DQ110_TimHandle, &oc, DQ110_PWM_CH) != HAL_OK) {
    APP_ErrorHandler();
  }

  /* 捕获通道：上升沿、直连 TI、不分频、无滤波（外部 4.7k 上拉 + 10nF 已硬件滤波） */
  ic.ICPolarity  = TIM_ICPOLARITY_RISING;
  ic.ICSelection = TIM_ICSELECTION_DIRECTTI;
  ic.ICPrescaler = TIM_ICPSC_DIV1;
  ic.ICFilter    = 0;
  if (HAL_TIM_IC_ConfigChannel(&DQ110_TimHandle, &ic, DQ110_CAP_CH) != HAL_OK) {
    APP_ErrorHandler();
  }

  /* 更新中断：tick32 扩展（优先级 1，低于捕获 0） */
  __HAL_TIM_ENABLE_IT(&DQ110_TimHandle, TIM_IT_UPDATE);
  HAL_NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);

  DQ110_TIM_PWM_Start();
  DQ110_TIM_CapStart();
}

/**
  * @brief  启动 PWM 输出（CCxE 置位）
  */
void DQ110_TIM_PWM_Start(void)
{
  if (HAL_TIM_PWM_Start(&DQ110_TimHandle, DQ110_PWM_CH) != HAL_OK) {
    APP_ErrorHandler();
  }
}

/**
  * @brief  停止 PWM 输出（CCxE 清位，输出通道关）
  */
void DQ110_TIM_PWM_Stop(void)
{
  HAL_TIM_PWM_Stop(&DQ110_TimHandle, DQ110_PWM_CH);
}

/**
  * @brief  设置占空比（0~DQ110_PWM_MAX）
  */
void DQ110_TIM_SetDuty(uint16_t duty)
{
  if (duty > DQ110_PWM_MAX) duty = DQ110_PWM_MAX;
  s_duty = duty;
  __HAL_TIM_SET_COMPARE(&DQ110_TimHandle, DQ110_PWM_CH, duty);
}

uint16_t DQ110_TIM_GetDuty(void)
{
  return s_duty;
}

/**
  * @brief  启动输入捕获中断（CC 中断，优先级 0 最高）
  */
void DQ110_TIM_CapStart(void)
{
  if (HAL_TIM_IC_Start_IT(&DQ110_TimHandle, DQ110_CAP_CH) != HAL_OK) {
    APP_ErrorHandler();
  }
}

/**
  * @brief  返回最近捕获的 32 位 tick（堵转检测：比较是否停住）
  */
uint32_t DQ110_TIM_GetLastCapTick(void)
{
  return s_last_cap_tick;
}

/**
  * @brief  TIM1 捕获中断回调（HAL_TIM_IRQHandler 调用）：更新实测转速
  */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  uint32_t t, dt;

  if (htim->Instance != DQ110_CAP_TIM) return;
  if (htim->Channel != HAL_TIM_ACTIVE_CHANNEL_1 && htim->Channel != HAL_TIM_ACTIVE_CHANNEL_3) return;

  /* 捕获时刻 = 更新中断扩展的 32 位 tick + 当前 CCR（误差 ≤50µs，可忽略） */
  t  = DQ110_CapTick32 + (uint32_t)__HAL_TIM_GET_COMPARE(htim, DQ110_CAP_CH);
  dt = t - s_last_cap_t;                      /* 无符号差：自动处理 tick32 回绕 */
  s_last_cap_t = t;
  s_last_cap_tick = t;

  if (dt > 0) {
    float f_hall = (float)DQ110_HSI_HZ / (float)dt;          /* 霍尔频率 Hz */
    g_dq110_rpm = 60.0f * f_hall / (float)DQ110_HALL_PPR;   /* RPM */
  }
  g_dq110_last_cap_ms = HAL_GetTick();
}
