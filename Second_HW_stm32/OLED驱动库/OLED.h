#ifndef __OLED_H
#define __OLED_H

#include "stm32f1xx_hal.h"   // 或者你工程里实际的 HAL 头文件
/*********************************************************************
 * 本文件基于江协科技 STM32 入门教程 OLED 驱动（标准库版）
 * 使用 AI 辅助移植为 HAL 库版本
 * 原始版权归江协科技所有
 * 详见：https://jiangxiekeji.com/tutorial/oled.html
 *********************************************************************/

void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);


#endif
