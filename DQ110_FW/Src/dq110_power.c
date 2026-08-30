/**
  ******************************************************************************
  * @file    dq110_power.c
  * @brief   低功耗：0 档 Stop 模式（待机 ≤5µA）+ 状态机转移
  *
  *         Stop 进入前：停 PWM、刷 IWDG 到满值、关 LSI（冻结 IWDG）
  *         Stop 唤醒（EXTI 下降沿）后：开 LSI 等就绪 → 刷 IWDG → ADC 重校准
  *
  *         状态机：STANDBY --档位有效--> RUN --过流/堵转--> FAULT --回0档--> STANDBY
  ******************************************************************************
  */
#include "main.h"
#include "dq110_power.h"
#include "dq110_protect.h"
#include "dq110_adc.h"
#include "dq110_tim.h"
#include "dq110_speed.h"

/**
  * @brief  Stop 模式参数（低功耗运行 LPR：唤醒后 Flash 延时 5µs，HSI 在 MR 后使能）
  */
void DQ110_PWR_Init(void)
{
  PWR_StopModeConfigTypeDef conf = {0};

  conf.LPVoltSelection      = PWR_STOPMOD_LPR_VOLT_SCALE2;
  conf.FlashDelay           = PWR_WAKEUP_FLASH_DELAY_5US;
  conf.WakeUpHsiEnableTime  = PWR_WAKEUP_HSIEN_AFTER_MR;
  conf.RegulatorSwitchDelay = PWR_WAKEUP_LPR_TO_MR_DELAY_2US;
  conf.SramRetentionVolt    = PWR_SRAM_RETENTION_VOLT_VOS;
  HAL_PWR_ConfigStopMode(&conf);
}

/**
  * @brief  进入 Stop 模式并等待 EXTI 唤醒；唤醒后恢复外设
  */
static void DQ110_EnterStop(void)
{
  /* 进入前：PWM 已停、占空比已清零（调用方保证） */
  HAL_SuspendTick();

  HAL_IWDG_Refresh(&DQ110_IwdgHandle);   /* 刷新到满值 */
  __HAL_RCC_LSI_DISABLE();               /* 关 LSI → IWDG 冻结（Stop 期间不复位） */

  HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

  /* ---- 唤醒（EXTI 下降沿）---- */
  HAL_ResumeTick();

  __HAL_RCC_LSI_ENABLE();
  while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) == RESET) {
  }
  HAL_IWDG_Refresh(&DQ110_IwdgHandle);   /* LSI 就绪后立即刷新（冻结期间未递减） */

  HAL_ADCEx_Calibration_Start(&DQ110_AdcHandle);  /* Stop 后 ADC 校准需重做 */
}

/**
  * @brief  状态机转移 + 低功耗管理（主循环调用）
  */
void DQ110_Power_Manage(void)
{
  uint32_t now = HAL_GetTick();

  switch (g_dq110_state) {
  case DQ110_STATE_STANDBY:
    /* 档位有效 → 启动马达 */
    if (g_dq110_gear != DQ110_GEAR_OFF) {
      DQ110_TIM_PWM_Start();
      DQ110_Speed_Reset();
      DQ110_Protect_StartRun(now);
      g_dq110_state = DQ110_STATE_RUN;
    }
    break;

  case DQ110_STATE_RUN:
    /* 回 0 档 → 停马达，进入 Stop 待机 */
    if (g_dq110_gear == DQ110_GEAR_OFF) {
      DQ110_TIM_SetDuty(0);
      DQ110_TIM_PWM_Stop();
      g_dq110_state = DQ110_STATE_STANDBY;
      DQ110_EnterStop();               /* 阻塞至 EXTI 唤醒；唤醒后状态仍为 STANDBY */
    }
    break;

  case DQ110_STATE_FAULT:
    /* 回 0 档清除故障 */
    if (g_dq110_gear == DQ110_GEAR_OFF) {
      DQ110_TIM_SetDuty(0);
      DQ110_TIM_PWM_Stop();
      DQ110_Speed_Reset();
      g_dq110_state = DQ110_STATE_STANDBY;
    }
    break;

  default:
    g_dq110_state = DQ110_STATE_STANDBY;
    break;
  }
}
