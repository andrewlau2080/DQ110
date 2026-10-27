/** DQ110 firmware V2.1, PY32F002B TSSOP20, 260916 requirements. */
#ifndef DQ110_H
#define DQ110_H

#include "py32f0xx_hal.h"

/* PY32F002B TSSOP20 F1 verified pinout: PB6(8)=SWDIO, PA2(18)=SWCLK,
 * PC0(4)=NRST; keep all three dedicated to SWD/reset. PB2(13)=SPI_SCK
 * is reserved and must not be reused for USB detection. */
#define DQ110_GEAR1_PORT       GPIOB
#define DQ110_GEAR1_PIN        GPIO_PIN_0
#define DQ110_GEAR2_PORT       GPIOB
#define DQ110_GEAR2_PIN        GPIO_PIN_1
#define DQ110_MT_EN_PORT       GPIOA
#define DQ110_MT_EN_PIN        GPIO_PIN_0
#define DQ110_BUCK_EN_PORT     GPIOA
#define DQ110_BUCK_EN_PIN      GPIO_PIN_1
#define DQ110_VBUS_DET_PORT    GPIOA
#define DQ110_VBUS_DET_PIN     GPIO_PIN_3  /* PA3 pin19; PB2 pin13 reserved SPI1_SCK */
#define DQ110_PWM_PORT         GPIOA
#define DQ110_PWM_PIN          GPIO_PIN_5
#define DQ110_BAT_ADC_PORT     GPIOA
#define DQ110_BAT_ADC_PIN      GPIO_PIN_6  /* ADC_IN3 */
#define DQ110_CUR_ADC_PORT     GPIOA
#define DQ110_CUR_ADC_PIN      GPIO_PIN_7  /* ADC_IN4 */

#define DQ110_HSI_HZ           24000000U
#define DQ110_PWM_FREQ_HZ      8000U
#define DQ110_PWM_RES          20U
#define DQ110_PWM_ISR_HZ       (DQ110_PWM_FREQ_HZ * DQ110_PWM_RES)
#define DQ110_PWM_PERIOD       (DQ110_HSI_HZ / DQ110_PWM_ISR_HZ - 1U)
#define DQ110_PWM_MAX          (DQ110_PWM_RES * 2U)

#define DQ110_GEAR1_DUTY       22U  /* calibrated at 3.0V for about 15RPM */
#define DQ110_GEAR2_DUTY       35U  /* calibrated at 3.0V for about 30RPM */
#define DQ110_SPEED_CAL_MV     3000U
#define DQ110_SPEED_COMP_MIN_MV 2500U
#define DQ110_RAMP_STEP        2U

#define DQ110_BAT_LOW_MV       2000U
#define DQ110_BAT_RESTART_MV   2200U
#define DQ110_BAT_SENSE_TOP_K  1000U
#define DQ110_BAT_SENSE_BOT_K  330U

/* Direct ADC current sensing: R2 = 15mΩ, with no current-sense amplifier.
 * DQ110_ADC_GetCurrentMv() samples ADC_IN4 and applies the 8-sample
 * moving average. Therefore the protection thresholds are the shunt
 * voltage itself: about 4.5mV and 9mV for 300mA and 600mA. A 1mV
 * practical margin is retained for ADC quantization/PWM ripple. */
#define DQ110_CUR_SHUNT_MOHM   15U
#define DQ110_CUR_GAIN_X       1U
#define DQ110_CUR_TRIP_MARGIN_MV 1U
#define DQ110_CUR_TRIP_G1_MA   300U
#define DQ110_CUR_TRIP_G2_MA   600U
#define DQ110_CUR_TRIP_MV(_ma) \
  ((((_ma) * DQ110_CUR_SHUNT_MOHM * DQ110_CUR_GAIN_X) + 500U) / 1000U + \
   DQ110_CUR_TRIP_MARGIN_MV)
#define DQ110_CUR_TRIP_G1_MV   4U  /* test threshold requested: low gear */
#define DQ110_CUR_TRIP_G2_MV   7U  /* test threshold requested: high gear */
#define DQ110_OC_BLANK_MS      600U
#define DQ110_OC_CONFIRM_CNT   10U

typedef enum { DQ110_GEAR_OFF = 0, DQ110_GEAR1, DQ110_GEAR2 } DQ110_Gear_t;
typedef enum {
  DQ110_SOURCE_NONE = 0,
  DQ110_SOURCE_BAT,
  DQ110_SOURCE_USB,
  DQ110_SOURCE_BAT_UNAVAILABLE
} DQ110_Source_t;

extern volatile DQ110_Gear_t   g_dq110_gear;
extern volatile DQ110_Source_t g_dq110_source;
extern volatile uint8_t         g_dq110_fault;

void APP_ErrorHandler(void);

#endif
