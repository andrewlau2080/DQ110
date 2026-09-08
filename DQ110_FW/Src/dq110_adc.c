/**
  ******************************************************************************
  * @file    dq110_adc.c
  * @brief   电池电压检测：采样 ADC 内部 VREFINT 通道(通道9)反推 VDD
  *
  *         原理：ADC 参考 = VCC(=电池电压)，VREFINT 恒 1.2V
  *               raw = VREFINT/VCC × 4095
  *               VCC_mv = 1200 × 4095 / raw = 4914000 / raw
  *         滤波：主循环每 100ms 采一次，8 点滑动平均
  ******************************************************************************
  */
#include "main.h"
#include "dq110_adc.h"

ADC_HandleTypeDef DQ110_AdcHandle;

static uint16_t s_raw_sum  = 0;    /* 滑动窗和（8 点 × raw，raw~1535@3.2V → 和 ~12280 不溢出 uint16? 12280<65535 ✓） */
static uint16_t s_raw_cnt  = 0;
static uint16_t s_vbat_mv  = DQ110_VBAT_BASE_MV;   /* 默认 3.2V（未采前按标定电压） */

/**
  * @brief  ADC 初始化：VREFINT 内部通道，单次转换（JTX018 已验证的 002B 配置）
  */
void DQ110_ADC_Init(void)
{
  ADC_ChannelConfTypeDef cc = {0};

  __HAL_RCC_ADC_FORCE_RESET();
  __HAL_RCC_ADC_RELEASE_RESET();
  __HAL_RCC_ADC_CLK_ENABLE();

  DQ110_AdcHandle.Instance = ADC1;
  if (HAL_ADCEx_Calibration_Start(&DQ110_AdcHandle) != HAL_OK) {
    APP_ErrorHandler();
  }

  DQ110_AdcHandle.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4; /* 24M/4=6MHz */
  DQ110_AdcHandle.Init.Resolution            = ADC_RESOLUTION_12B;
  DQ110_AdcHandle.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
  DQ110_AdcHandle.Init.ScanConvMode          = ADC_SCAN_DIRECTION_FORWARD;
  DQ110_AdcHandle.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
  DQ110_AdcHandle.Init.LowPowerAutoWait      = DISABLE;
  DQ110_AdcHandle.Init.ContinuousConvMode    = DISABLE;
  DQ110_AdcHandle.Init.DiscontinuousConvMode = DISABLE;
  DQ110_AdcHandle.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
  DQ110_AdcHandle.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
  DQ110_AdcHandle.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;
  DQ110_AdcHandle.Init.SamplingTimeCommon    = ADC_SAMPLETIME_13CYCLES_5;
  if (HAL_ADC_Init(&DQ110_AdcHandle) != HAL_OK) {
    APP_ErrorHandler();
  }

  /* VREFINT 内部通道（002B = ADC_CHANNEL_9） */
  cc.Rank    = ADC_RANK_CHANNEL_NUMBER;
  cc.Channel = ADC_CHANNEL_VREFINT;
  if (HAL_ADC_ConfigChannel(&DQ110_AdcHandle, &cc) != HAL_OK) {
    APP_ErrorHandler();
  }
}

/**
  * @brief  主循环 100ms 调用：采一次 VREFINT，8 点滑动平均后更新 s_vbat_mv
  * @retval 1 = 本次有更新；0 = 尚未采满（滤波窗未满）
  */
uint8_t DQ110_ADC_Update(void)
{
  uint16_t raw;
  uint32_t v;

  if (HAL_ADC_Start(&DQ110_AdcHandle) != HAL_OK) {
    return 0;
  }
  if (HAL_ADC_PollForConversion(&DQ110_AdcHandle, 10) != HAL_OK) {
    HAL_ADC_Stop(&DQ110_AdcHandle);
    return 0;
  }
  raw = (uint16_t)HAL_ADC_GetValue(&DQ110_AdcHandle);
  HAL_ADC_Stop(&DQ110_AdcHandle);

  /* 滑动平均（8 点） */
  if (s_raw_cnt < 8U) {
    s_raw_sum += raw;
    s_raw_cnt++;
    if (s_raw_cnt < 8U) {
      return 0;
    }
  } else {
    /* 窗满后：减最老? 简化：直接全窗平均用 8 点滚动累加需环形；改用低通更简单：
     * 这里用「去最老」不便，采用指数平均：vbat = (7*旧 + 新)/8 */
    s_raw_sum = (uint16_t)(((uint32_t)s_raw_sum * 7U + (uint32_t)raw) / 8U);
    s_raw_cnt  = 8U;
  }

  if (s_raw_sum == 0U) {
    return 0;
  }
  if (s_raw_cnt >= 8U) {
    /* s_raw_sum 已是指数平均后的 raw 均值（×1），直接反推 */
    v = (uint32_t)DQ110_VREFINT_MV * 4095U / s_raw_sum;
    if (v > 3400U) v = 3400U;         /* 防异常 */
    if (v < 2400U) v = 2400U;
    s_vbat_mv = (uint16_t)v;
    return 1;
  }
  return 0;
}

/**
  * @brief  返回滤波后的电池电压 mV
  */
uint16_t DQ110_ADC_GetVbatMv(void)
{
  return s_vbat_mv;
}

/************************ (C) COPYRIGHT Puya *****END OF FILE****/
