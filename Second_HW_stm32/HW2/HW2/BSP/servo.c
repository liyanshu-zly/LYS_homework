#include "servo.h"
#include "tim.h"

/*********************************************************************
 * 舵机驱动实现
 *
 *   TIM3_CH1 -> PA6，PSC = 71，ARR = 19999
 *   计数频率 = 72MHz / 72 = 1MHz，即 CCR 的数值就是脉宽微秒数
 *********************************************************************/

static float servo_angle = 90.0f;

void Servo_Init(void)
{
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);   /* 使能 TIM3 计数器 + CH1 输出 */
    Servo_SetAngle(90.0f);                      /* 上电回中位 */
}

void Servo_SetPulse(uint16_t PulseUs)
{
    if (PulseUs < SERVO_MIN_US) PulseUs = SERVO_MIN_US;
    if (PulseUs > SERVO_MAX_US) PulseUs = SERVO_MAX_US;

    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, PulseUs);
}

void Servo_SetAngle(float Angle)
{
    float pulse;

    if (Angle < 0.0f)   Angle = 0.0f;
    if (Angle > 180.0f) Angle = 180.0f;

    servo_angle = Angle;

    /* 0° -> 500us，180° -> 2500us，线性插值 */
    pulse = SERVO_MIN_US + Angle * (SERVO_MAX_US - SERVO_MIN_US) / 180.0f;

    Servo_SetPulse((uint16_t)(pulse + 0.5f));   /* +0.5 做四舍五入 */
}

float Servo_GetAngle(void)
{
    return servo_angle;
}
