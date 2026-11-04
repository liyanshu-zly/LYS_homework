#ifndef __ENCODER_H
#define __ENCODER_H

#include "main.h"

/*********************************************************************
 * 旋转编码器（EC11）+ 按键 BSP
 *
 * 引脚全部在 CubeMX（HW1.ioc）里配置，不再在这里重复定义：
 *   PA0  -> ENC_KEY   EXTI0，下降沿，上拉
 *   PA8  -> ENC_B     EXTI8，双边沿，上拉
 *   PA9  -> ENC_A     EXTI9，双边沿，上拉
 *
 * CubeMX 生成的宏在 main.h 里：
 *   ENC_KEY_Pin / ENC_KEY_GPIO_Port
 *   ENC_B_Pin   / ENC_B_GPIO_Port
 *   ENC_A_Pin   / ENC_A_GPIO_Port
 *********************************************************************/

/* 一圈的计数（边沿数）：EC11 每圈 20 个定位点，每个定位点 4 个边沿 */
#define ENC_EDGES_PER_REV  80

void     Encoder_Init(void);

int32_t  Encoder_GetStep(void);      /* 累计边沿数，正=CW，负=CCW */
void     Encoder_ResetStep(void);

uint32_t Encoder_GetKeyCount(void);  /* 按键按下次数 */
void     Encoder_ResetKeyCount(void);

#endif
