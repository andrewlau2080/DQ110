/**
  ******************************************************************************
  * @file    dq110_speed.c
  * @brief   转速闭环：PID 稳速（一档 15RPM / 二档 30RPM）
  *
  *         位置式 PID + 积分限幅 + 输出限幅，控制周期 100ms。
  *         占空比输出范围 DQ110_PWM_DUTY_MIN ~ DQ110_PWM_MAX。
  *         实测转速来自 g_dq110_rpm（霍尔捕获中断更新，慢速信号保持旧值，
  *         PID 对保持值积分，靠小 Kp/Ki 防过冲；参数在标定阶段调整）。
  ******************************************************************************
  */
#include "main.h"
#include "dq110_speed.h"
#include "dq110_tim.h"
#include "dq110_adc.h"

#ifndef DQ110_NO_HALL
/* ==================== 霍尔闭环版：PID 稳速 ==================== */

static float s_prev_err   = 0.0f;
static float s_integral   = 0.0f;
static float s_last_target = -1.0f;   /* 目标变化检测 */

/**
  * @brief  PID 状态清零
  */
void DQ110_Speed_Reset(void)
{
  s_prev_err = 0.0f;
  s_integral = 0.0f;
  s_last_target = -1.0f;
}

/**
  * @brief  转速控制（主循环调用，100ms 节拍）
  */
void DQ110_Speed_Control(uint32_t now_ms)
{
  static uint32_t s_last_ctrl = 0;
  float target, err, out;
  uint16_t duty;

  if ((now_ms - s_last_ctrl) < DQ110_CTRL_PERIOD_MS) return;
  s_last_ctrl = now_ms;

  /* 目标转速按当前档位 */
  target = (g_dq110_gear == DQ110_GEAR2) ? DQ110_GEAR2_RPM :
           (g_dq110_gear == DQ110_GEAR1) ? DQ110_GEAR1_RPM : 0.0f;

  /* 非运行 / 目标为 0：占空比归零 */
  if (g_dq110_state != DQ110_STATE_RUN || target <= 0.0f) {
    DQ110_TIM_SetDuty(0);
    s_prev_err = 0.0f;
    s_integral = 0.0f;
    s_last_target = target;
    return;
  }

  /* 换挡（目标变化）时清积分，避免过冲 */
  if (target != s_last_target) {
    s_prev_err = 0.0f;
    s_integral = 0.0f;
    s_last_target = target;
  }

  err = target - g_dq110_rpm;
  s_integral += err * ((float)DQ110_CTRL_PERIOD_MS / 1000.0f);
  /* 积分限幅：±最大占空比一半 */
  if (s_integral >  (float)DQ110_PWM_MAX / 2.0f) s_integral =  (float)DQ110_PWM_MAX / 2.0f;
  if (s_integral < -(float)DQ110_PWM_MAX / 2.0f) s_integral = -(float)DQ110_PWM_MAX / 2.0f;

  out = DQ110_PID_KP * err + DQ110_PID_KI * s_integral + DQ110_PID_KD * (err - s_prev_err);
  s_prev_err = err;

  /* 输出钳位到 [最小占空比, 最大占空比]（最小钳位保证马达克服静摩擦起转） */
  if (out < (float)DQ110_PWM_DUTY_MIN) out = (float)DQ110_PWM_DUTY_MIN;
  if (out > (float)DQ110_PWM_MAX)      out = (float)DQ110_PWM_MAX;
  duty = (uint16_t)out;

  DQ110_TIM_SetDuty(duty);
}

#else  /* DQ110_NO_HALL */
/* ==================== 低成本版（无霍尔）：电压补偿开环恒速 ====================
 * PWM = 基础占空比（满电 3.0V 标定） × 满电电压 / 当前电池电压
 * 电池电压 = VREFINT(1200mV) × 4095 / adc_vrefint_raw（ADC 参考即 VDD）
 * 电压下降 → PWM 自动加大 → 转速保持恒定；补偿到 100% 仍不够时由低压保护接管
 * ======================================================================== */

static uint16_t s_last_base = 0;   /* 上次基础占空比（换挡检测） */

void DQ110_Speed_Reset(void)
{
  s_last_base = 0;
}

void DQ110_Speed_Control(uint32_t now_ms)
{
  static uint32_t s_last_ctrl = 0;
  uint16_t base, vref;
  uint32_t vbat_mv, duty;

  if ((now_ms - s_last_ctrl) < DQ110_CTRL_PERIOD_MS) return;
  s_last_ctrl = now_ms;

  /* 非运行：占空比归零 */
  if (g_dq110_state != DQ110_STATE_RUN || g_dq110_gear == DQ110_GEAR_OFF) {
    DQ110_TIM_SetDuty(0);
    s_last_base = 0;
    return;
  }

  /* 基础占空比（满电 3.0V 标定值，按档位） */
  base = (g_dq110_gear == DQ110_GEAR2) ? DQ110_DUTY_BASE_G2 : DQ110_DUTY_BASE_G1;
  if (base != s_last_base) {           /* 换挡：立即切换占空比 */
    s_last_base = base;
  }

  /* 采 VREFINT 反推电池电压（ADC 参考 = VDD） */
  vref = DQ110_ADC_ReadVrefintRaw();
  if (vref == 0) return;               /* 采样失败保持当前占空比 */
  vbat_mv = (uint32_t)DQ110_VREFINT_MV * 4095U / (uint32_t)vref;

  /* 电压前馈补偿：电压下降 → 占空比同比例加大（线性） */
  duty = (uint32_t)base * DQ110_VBAT_FULL_MV / vbat_mv;

  /* 钳位 [最小占空比, 100%] */
  if (duty < DQ110_PWM_DUTY_MIN) duty = DQ110_PWM_DUTY_MIN;
  if (duty > DQ110_PWM_MAX)      duty = DQ110_PWM_MAX;

  DQ110_TIM_SetDuty((uint16_t)duty);
}
#endif /* DQ110_NO_HALL */
