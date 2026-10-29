#include "main.h"
#include "dq110_adc.h"

ADC_HandleTypeDef DQ110_AdcHandle;

typedef struct {
  uint16_t value[8];
  uint32_t sum;
  uint8_t index;
  uint8_t ready;
} DQ110_Filter8_t;

static DQ110_Filter8_t s_bat_filter;
static DQ110_Filter8_t s_vbus_filter;
static DQ110_Filter8_t s_cur_filter;
static DQ110_Filter8_t s_vcc_filter;

/* One protection tick must cover at least one 8kHz PWM period (125us),
 * otherwise a fixed 10ms sampling phase can repeatedly hit the PWM-off
 * interval and report zero current. */
#define DQ110_CUR_ADC_SAMPLE_COUNT 32U

static void DQ110_ADC_WaitPwmOn(void)
{
  uint32_t guard = 2000U;
  while (guard-- != 0U) {
    if (HAL_GPIO_ReadPin(DQ110_PWM_PORT, DQ110_PWM_PIN) == GPIO_PIN_SET) {
      return;
    }
  }
}

/* Keep the validated 8-sample moving-average behavior. */
static uint16_t DQ110_Filter8_Update(DQ110_Filter8_t *f, uint16_t sample)
{
  uint8_t i;
  if (!f->ready) {
    for (i = 0U; i < 8U; i++) f->value[i] = sample;
    f->sum = (uint32_t)sample * 8U;
    f->index = 0U;
    f->ready = 1U;
    return sample;
  }
  f->sum -= f->value[f->index];
  f->value[f->index] = sample;
  f->sum += sample;
  f->index = (uint8_t)((f->index + 1U) & 7U);
  return (uint16_t)(f->sum / 8U);
}

static uint16_t DQ110_ADC_Conv(uint32_t channel)
{
  ADC_ChannelConfTypeDef c = {0};
  if ((DQ110_AdcHandle.Instance->CR & ADC_CR_ADEN) != 0U) {
    DQ110_AdcHandle.Instance->CR |= ADC_CR_ADDIS;
    while ((DQ110_AdcHandle.Instance->CR & ADC_CR_ADDIS) != 0U) { }
  }
  DQ110_AdcHandle.Instance->CHSELR = 0U;
  c.Rank = ADC_RANK_CHANNEL_NUMBER;
  c.Channel = channel;
  if (HAL_ADC_ConfigChannel(&DQ110_AdcHandle, &c) != HAL_OK) return 0U;
  if (HAL_ADC_Start(&DQ110_AdcHandle) != HAL_OK) return 0U;
  if (HAL_ADC_PollForConversion(&DQ110_AdcHandle, 10U) != HAL_OK) {
    HAL_ADC_Stop(&DQ110_AdcHandle);
    return 0U;
  }
  c.Channel = 0U;
  {
    uint16_t v = (uint16_t)HAL_ADC_GetValue(&DQ110_AdcHandle);
    HAL_ADC_Stop(&DQ110_AdcHandle);
    return v;
  }
}

void DQ110_ADC_Init(void)
{
  ADC_ChannelConfTypeDef c = {0};
  __HAL_RCC_ADC_FORCE_RESET();
  __HAL_RCC_ADC_RELEASE_RESET();
  __HAL_RCC_ADC_CLK_ENABLE();
  DQ110_AdcHandle.Instance = ADC1;
  DQ110_AdcHandle.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  DQ110_AdcHandle.Init.Resolution = ADC_RESOLUTION_12B;
  DQ110_AdcHandle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  DQ110_AdcHandle.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
  DQ110_AdcHandle.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  DQ110_AdcHandle.Init.LowPowerAutoWait = DISABLE;
  DQ110_AdcHandle.Init.ContinuousConvMode = DISABLE;
  DQ110_AdcHandle.Init.DiscontinuousConvMode = DISABLE;
  DQ110_AdcHandle.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  DQ110_AdcHandle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  DQ110_AdcHandle.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  DQ110_AdcHandle.Init.SamplingTimeCommon = ADC_SAMPLETIME_13CYCLES_5;
  if (HAL_ADC_Init(&DQ110_AdcHandle) != HAL_OK ||
      HAL_ADCEx_Calibration_Start(&DQ110_AdcHandle) != HAL_OK) APP_ErrorHandler();
  c.Rank = ADC_RANK_CHANNEL_NUMBER;
  c.Channel = ADC_CHANNEL_VREFINT;
  if (HAL_ADC_ConfigChannel(&DQ110_AdcHandle, &c) != HAL_OK) APP_ErrorHandler();
}

uint16_t DQ110_ADC_GetVccMv(void)
{
  uint16_t raw = DQ110_ADC_Conv(ADC_CHANNEL_VREFINT);
  if (!raw) return 0U;
  raw = DQ110_Filter8_Update(&s_vcc_filter, raw);
  return (uint16_t)(1200U * 4095U / raw);
}

static uint16_t DQ110_ADC_PinMv(uint32_t channel)
{
  uint16_t raw = DQ110_ADC_Conv(channel);
  uint16_t vcc = DQ110_ADC_GetVccMv();
  return (raw && vcc) ? (uint16_t)((uint32_t)raw * vcc / 4095U) : 0U;
}

uint16_t DQ110_ADC_GetBatteryMv(void)
{
  uint16_t pin_mv = DQ110_ADC_PinMv(ADC_CHANNEL_3);
  uint16_t bat_mv;
  if (!pin_mv) return 0U;
  bat_mv = (uint16_t)((uint32_t)pin_mv *
                      (DQ110_BAT_SENSE_TOP_K + DQ110_BAT_SENSE_BOT_K) /
                      DQ110_BAT_SENSE_BOT_K);
  return DQ110_Filter8_Update(&s_bat_filter, bat_mv);
}

uint16_t DQ110_ADC_GetVbusDetMv(void)
{
  uint16_t det_mv = DQ110_ADC_PinMv(ADC_CHANNEL_1);
  if (!det_mv) return 0U;
  return DQ110_Filter8_Update(&s_vbus_filter, det_mv);
}

uint16_t DQ110_ADC_GetCurrentMv(void)
{
  uint32_t sum = 0U;
  uint16_t raw;
  uint16_t vcc;
  uint16_t current_mv;
  uint32_t i;

  /* Direct ADC_IN4 sampling: average a complete burst before applying the
   * longer 8-tick moving average used by the protection logic. */
  for (i = 0U; i < DQ110_CUR_ADC_SAMPLE_COUNT; i++) {
    /* PA5 high is the software-PWM motor-on phase. Do not sample the
     * low-side shunt during the PWM-off interval. */
    DQ110_ADC_WaitPwmOn();
    sum += DQ110_ADC_Conv(ADC_CHANNEL_4);
  }
  raw = (uint16_t)((sum + (DQ110_CUR_ADC_SAMPLE_COUNT / 2U)) /
                   DQ110_CUR_ADC_SAMPLE_COUNT);
  vcc = DQ110_ADC_GetVccMv();
  current_mv = (raw && vcc) ?
               (uint16_t)((uint32_t)raw * vcc / 4095U) : 0U;
  if (!current_mv) return 0U;
  return DQ110_Filter8_Update(&s_cur_filter, current_mv);
}
