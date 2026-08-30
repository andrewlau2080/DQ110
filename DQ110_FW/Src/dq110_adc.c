/**
  ******************************************************************************
  * @file    dq110_adc.c
  * @brief   过流检测 ADC（采样电阻 + 运放 → 12 位 ADC）
  *          阈值换算：V_oc = I × R_sense × G → LSB = V_oc × 4095 / VDD
  *          例：0.65A × 0.1Ω × 1 / 3.0V × 4095 ≈ 88 LSB；1.3A ≈ 177 LSB
  ******************************************************************************
  */
#include "main.h"
#include "dq110_adc.h"

ADC_HandleTypeDef DQ110_AdcHandle;

/* 8 点滑动平均缓冲 */
static uint16_t s_fifo[DQ110_OC_FILTER_N];
static uint8_t  s_fifo_idx = 0;
static uint8_t  s_fifo_cnt = 0;

/**
  * @brief  ADC1 初始化：12 位，软件触发单次转换
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

  DQ110_AdcHandle.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4; /* 24M/4 = 6MHz */
  DQ110_AdcHandle.Init.Resolution            = ADC_RESOLUTION_12B;
  DQ110_AdcHandle.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
  DQ110_AdcHandle.Init.ScanConvMode          = ADC_SCAN_DIRECTION_FORWARD;
  DQ110_AdcHandle.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
  DQ110_AdcHandle.Init.LowPowerAutoWait      = ENABLE;
  DQ110_AdcHandle.Init.ContinuousConvMode    = DISABLE;
  DQ110_AdcHandle.Init.DiscontinuousConvMode = DISABLE;
  DQ110_AdcHandle.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
  DQ110_AdcHandle.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
  DQ110_AdcHandle.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;
  DQ110_AdcHandle.Init.SamplingTimeCommon    = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_Init(&DQ110_AdcHandle) != HAL_OK) {
    APP_ErrorHandler();
  }

  cc.Rank    = ADC_RANK_CHANNEL_NUMBER;
  cc.Channel = DQ110_OC_ADC_CH;
  if (HAL_ADC_ConfigChannel(&DQ110_AdcHandle, &cc) != HAL_OK) {
    APP_ErrorHandler();
  }
}

/**
  * @brief  单次转换读取原始值
  */
uint16_t DQ110_ADC_ReadRaw(void)
{
  if (HAL_ADC_Start(&DQ110_AdcHandle) != HAL_OK) return 0;
  if (HAL_ADC_PollForConversion(&DQ110_AdcHandle, 10) != HAL_OK) return 0;
  return (uint16_t)HAL_ADC_GetValue(&DQ110_AdcHandle);
}

/**
  * @brief  8 点滑动平均（每 10ms 采一点，80ms 窗口；配合连续超阈值判定，
  *         过流保护响应 ≤100ms，满足规格书）
  */
uint16_t DQ110_ADC_ReadFiltered(void)
{
  uint32_t sum = 0;
  uint8_t  i;

  s_fifo[s_fifo_idx] = DQ110_ADC_ReadRaw();
  s_fifo_idx = (s_fifo_idx + 1U) % DQ110_OC_FILTER_N;
  if (s_fifo_cnt < DQ110_OC_FILTER_N) s_fifo_cnt++;

  for (i = 0; i < s_fifo_cnt; i++) sum += s_fifo[i];
  return (uint16_t)(sum / s_fifo_cnt);
}

/**
  * @brief  电流阈值（mA）→ ADC 原始值 LSB
  */
uint32_t DQ110_OC_ThresholdLsb(uint16_t current_ma)
{
  float v_oc = (float)current_ma * 0.001f * DQ110_OC_SENSE_R_OHM * DQ110_OC_AMP_GAIN;
  return (uint32_t)(v_oc * 4095.0f / ((float)DQ110_ADC_VREF_MV / 1000.0f));
}
