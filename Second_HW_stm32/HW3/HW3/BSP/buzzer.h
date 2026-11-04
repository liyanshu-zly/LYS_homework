#ifndef __BUZZER_H
#define __BUZZER_H

#include "main.h"

/*********************************************************************
 * 蜂鸣器驱动（TIM2_CH2 -> PA1）
 *
 *   TIM2 计数时钟 = 72MHz，PSC = 71 -> 1MHz，ARR = 999 -> 1ms
 *   即：PWM 方波 1kHz，同时每个周期（1ms）产生一次更新中断。
 *
 *   HW3 用"按需响一次"的接口：
 *     收到有效串口数据 -> Buzzer_Beep(200)
 *     编码器按键按下   -> Buzzer_Beep(200)
 *   计时由 1ms 中断倒计时，主循环完全不被阻塞。
 *********************************************************************/

void    Buzzer_Init(void);          /* 启动 PWM + 打开 1ms 更新中断 */
void    Buzzer_Beep(uint16_t Ms);   /* 响 Ms 毫秒，重复调用会重新计时 */
void    Buzzer_Off(void);
uint8_t Buzzer_IsOn(void);          /* 1 = 正在响 */
void    Buzzer_Tick(void);          /* 每 1ms 调用一次（TIM2 中断里） */

#endif
