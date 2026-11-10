#ifndef __APP_H
#define __APP_H

#include "main.h"

/* 上电初始化：启动 PWM / 两路编码器 / 10ms 控制中断 / OLED */
void App_Init(void);

/* 主循环里调用：只刷 OLED，控制全部在 TIM4 中断里 */
void App_Loop(void);

/* 在 HAL_TIM_PeriodElapsedCallback() 里调用，只处理 TIM4 */
void App_TimerISR(TIM_HandleTypeDef *htim);

#endif
