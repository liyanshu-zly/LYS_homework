#ifndef __SERVO_H
#define __SERVO_H

#include "main.h"

/*********************************************************************
 * 舵机 PWM 驱动（TIM3_CH1 -> PA6）
 *
 *   时钟：TIM3 计数时钟 = 72MHz，PSC = 71  ->  1MHz（1 个计数 = 1us）
 *   周期：ARR = 19999  ->  20000us = 20ms  ->  50Hz
 *   脉宽：500 ~ 2500us 对应 0 ~ 180°
 *
 *   为什么用 TIM3 而不是 TIM2：TIM2 已经被蜂鸣器占用，
 *   同一个定时器的所有通道共用一个 PSC/ARR，舵机要 50Hz、
 *   蜂鸣器要 1kHz，必须分两个定时器。
 *********************************************************************/

#define SERVO_MIN_US   500      /* 0° 对应脉宽 */
#define SERVO_MAX_US   2500     /* 180° 对应脉宽 */

void  Servo_Init(void);                  /* 启动 PWM，回到 90° 中位 */
void  Servo_SetAngle(float Angle);       /* 0 ~ 180 度，超范围自动限幅 */
void  Servo_SetPulse(uint16_t PulseUs);  /* 直接给 500 ~ 2500us */
float Servo_GetAngle(void);              /* 读回当前角度 */

#endif
