/**
  ******************************************************************************
  * @file    dq110.h
  * @brief   DQ110 双速直流马达驱动器 - 固件全局定义
  *          引脚映射/系统参数/状态机，对应《DQ110_产品规格书.md》7.4(TSSOP20)/7.5(SOP8)
  ******************************************************************************
  */
#ifndef DQ110_H
#define DQ110_H

#include "py32f0xx_hal.h"

/* =====================================================================
 * 封装选择
 *   默认编译目标：TSSOP20 主设计（PY32F002AF15P）
 *   CMake -DDQ110_PKG=SOP8 切换低成本版（PY32F002AL15S，单 MOS 驱动）
 * ===================================================================== */
#ifdef DQ110_PKG_SOP8
  /* ---------- SOP8 版（规格书 7.5） ---------- */
  #define DQ110_PWM_TIM              TIM1
  #define DQ110_PWM_CH               TIM_CHANNEL_2
  #define DQ110_PWM_GPIO_PORT        GPIOA
  #define DQ110_PWM_GPIO_PIN         GPIO_PIN_13   /* pin5：复位默认 SWDIO，运行时释放为 PWM */
  #define DQ110_PWM_AF               GPIO_AF13_TIM1

  #define DQ110_CAP_TIM              TIM1
  #define DQ110_CAP_CH               TIM_CHANNEL_1
  #define DQ110_CAP_GPIO_PORT        GPIOA
  #define DQ110_CAP_GPIO_PIN         GPIO_PIN_4    /* pin2：HALL，TIM1_CH1 输入捕获 */
  #define DQ110_CAP_AF               GPIO_AF13_TIM1

  #define DQ110_SW1_GPIO_PORT        GPIOA
  #define DQ110_SW1_GPIO_PIN         GPIO_PIN_3    /* pin3：一档 */
  #define DQ110_SW2_GPIO_PORT        GPIOB
  #define DQ110_SW2_GPIO_PIN         GPIO_PIN_3    /* pin4：二档（复位默认 SWCLK，运行释放） */
  /* SOP8 的 SW1/SW2 都是 EXTI 线3 → EXTI2_3_IRQn */
  #define DQ110_EXTI_SW_IRQ          EXTI2_3_IRQn
  #define DQ110_EXTI_SW_LINES        (GPIO_PIN_3)

  #define DQ110_OC_ADC_CH            ADC_CHANNEL_1
  #define DQ110_OC_GPIO_PORT         GPIOA
  #define DQ110_OC_GPIO_PIN          GPIO_PIN_1    /* pin7：过流检测（采样电阻+运放输出） */
#else
  /* ---------- TSSOP20 版（规格书 7.4，默认） ---------- */
  #define DQ110_PWM_TIM              TIM1
  #define DQ110_PWM_CH               TIM_CHANNEL_1
  #define DQ110_PWM_GPIO_PORT        GPIOA
  #define DQ110_PWM_GPIO_PIN         GPIO_PIN_4    /* pin15：M- 驱动，TIM1_CH1 PWM → MOS 栅极 */
  #define DQ110_PWM_AF               GPIO_AF13_TIM1

  #define DQ110_CAP_TIM              TIM1
  #define DQ110_CAP_CH               TIM_CHANNEL_3
  #define DQ110_CAP_GPIO_PORT        GPIOB
  #define DQ110_CAP_GPIO_PIN         GPIO_PIN_6    /* pin5：HALL，TIM1_CH3 输入捕获，外部上拉 4.7k */
  #define DQ110_CAP_AF               GPIO_AF1_TIM1

  #define DQ110_SW1_GPIO_PORT        GPIOA
  #define DQ110_SW1_GPIO_PIN         GPIO_PIN_2    /* pin13：一档，EXTI2 唤醒 */
  #define DQ110_SW2_GPIO_PORT        GPIOA
  #define DQ110_SW2_GPIO_PIN         GPIO_PIN_5    /* pin7：二档，EXTI5 唤醒 */
  /* TSSOP20：SW1=EXTI2、SW2=EXTI5 → 两个中断 */
  #define DQ110_EXTI_SW_IRQ          EXTI2_3_IRQn
  #define DQ110_EXTI_SW_LINES        (GPIO_PIN_2)
  #define DQ110_EXTI_SW2_IRQ         EXTI4_15_IRQn
  #define DQ110_EXTI_SW2_LINES       (GPIO_PIN_5)

  #define DQ110_OC_ADC_CH            ADC_CHANNEL_6
  #define DQ110_OC_GPIO_PORT         GPIOA
  #define DQ110_OC_GPIO_PIN          GPIO_PIN_6    /* pin6：过流检测（采样电阻+运放输出） */
#endif

/* =====================================================================
 * 系统时钟与 PWM
 * ===================================================================== */
#define DQ110_HSI_HZ                 24000000U     /* HSI 配置为 24MHz */
#define DQ110_PWM_FREQ_HZ            20000U        /* PWM 频率 20kHz（听域以上，MOS 开关损耗合理） */
#define DQ110_PWM_PERIOD             (DQ110_HSI_HZ / DQ110_PWM_FREQ_HZ - 1U)  /* ARR = 1199 */
#define DQ110_PWM_MAX                (DQ110_PWM_PERIOD + 1U)                  /* 占空比分辨率 1200 */
#define DQ110_PWM_DUTY_MIN           (DQ110_PWM_MAX * 20 / 100)               /* PID 最小输出钳位 20% */

/* =====================================================================
 * 档位目标转速（RPM）
 * ===================================================================== */
#define DQ110_GEAR1_RPM              15.0f
#define DQ110_GEAR2_RPM              30.0f

/* =====================================================================
 * 霍尔测速
 * ===================================================================== */
#define DQ110_HALL_PPR               1             /* 霍尔每转脉冲数：按实际马达极对数/减速比标定修改 */

/* =====================================================================
 * 过流保护（采样电阻 0.1Ω + 运放，ADC 通道见封装宏）
 * ===================================================================== */
#define DQ110_OC_GEAR1_MA            650U          /* 一档过流阈值 0.65A */
#define DQ110_OC_GEAR2_MA            1300U         /* 二档过流阈值 1.3A */
#define DQ110_OC_SENSE_R_OHM         0.1f          /* 采样电阻 */
#define DQ110_OC_AMP_GAIN            1.0f          /* 运放增益：按原理图 BOM 实际值修改 */
#define DQ110_ADC_VREF_MV            3000U         /* ADC 参考 = VDD，标定基准 3.0V（额定） */
#define DQ110_OC_FILTER_N            8             /* 过流滑动平均点数（10ms/点 → 80ms 窗口） */
#define DQ110_OC_TRIP_N              4             /* 连续超阈值次数判定保护（40ms） */

/* =====================================================================
 * 堵转/启动
 * ===================================================================== */
#define DQ110_STALL_MS               500U          /* 运行中超过该时间无霍尔脉冲 → 堵转关断 */
#define DQ110_START_GRACE_MS         1000U         /* 启动/换挡后宽限期（马达起转需要时间） */

/* =====================================================================
 * 转速闭环 PID（增量式，控制周期 100ms）
 * ===================================================================== */
#define DQ110_CTRL_PERIOD_MS         100U
#define DQ110_PID_KP                 0.05f         /* 标定后调整 */
#define DQ110_PID_KI                 0.01f
#define DQ110_PID_KD                 0.0f
#define DQ110_PID_OUT_LIMIT          (DQ110_PWM_MAX / 10)  /* 单控制周期占空比变化限幅（10%） */

/* =====================================================================
 * 状态机
 * ===================================================================== */
typedef enum {
  DQ110_GEAR_OFF = 0,   /* 0 档：SW1/SW2 均高 */
  DQ110_GEAR1,          /* 一档：SW1 低（15 RPM） */
  DQ110_GEAR2           /* 二档：SW2 低（30 RPM） */
} DQ110_Gear_t;

typedef enum {
  DQ110_STATE_STANDBY = 0,  /* 0 档待机（Stop 模式） */
  DQ110_STATE_RUN,          /* 马达运行（PID 闭环） */
  DQ110_STATE_FAULT         /* 过流/堵转保护关断，回 0 档清除 */
} DQ110_State_t;

/* =====================================================================
 * 模块间共享全局
 * ===================================================================== */
extern volatile DQ110_Gear_t   g_dq110_gear;        /* 当前档位（主循环防抖确认） */
extern volatile DQ110_State_t  g_dq110_state;       /* 运行状态 */
extern volatile uint8_t        g_dq110_gear_event;  /* EXTI 档位变化事件（主循环扫描） */
extern volatile uint16_t       g_dq110_duty;        /* 当前 PWM 占空比 0~DQ110_PWM_MAX */
extern volatile float          g_dq110_rpm;         /* 实测转速（霍尔捕获更新） */
extern volatile uint32_t       g_dq110_last_cap_ms; /* 最近一次霍尔捕获的 HAL_GetTick() */

/* 公共错误处理 */
void APP_ErrorHandler(void);

#endif /* DQ110_H */
