#ifndef __BUZZER_H
#define __BUZZER_H

#include "main.h"

/*********************************************************************
 * 蜂鸣器驱动（TIM2_CH2 -> PA1）
 *
 *   TIM2 计数时钟 = 72MHz，PSC = 71 -> 1MHz，ARR = 999 -> 1ms
 *   即：PWM 方波 1kHz，同时每个周期（1ms）产生一次更新中断。
 *
 *   节奏：每 1000ms 一个周期，前 200ms 响，后 800ms 停
 *         —— 即"每隔 1s 响 0.2s"
 *
 *   Buzzer_Tick() 必须在 TIM2 的更新中断里被调用
 *   （main.c 的 HAL_TIM_PeriodElapsedCallback 里调用）。
 *********************************************************************/

void    Buzzer_Init(void);      /* 启动 PWM + 打开 1ms 更新中断 */
void    Buzzer_On(void);
void    Buzzer_Off(void);
uint8_t Buzzer_IsOn(void);      /* 1 = 正在响 */
void    Buzzer_Tick(void);      /* 每 1ms 调用一次 */

#endif
