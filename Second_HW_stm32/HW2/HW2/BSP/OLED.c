#include "OLED.h"
#include "OLED_Font.h"
#include "gpio.h"
#include "main.h"
/*********************************************************************
 * 本文件基于江协科技 STM32 入门教程 OLED 驱动（标准库版）
 * 使用 AI 辅助移植为 HAL 库版本
 * 原始版权归江协科技所有
 * 详见：https://jiangxiekeji.com/tutorial/oled.html
 *********************************************************************/
/* ==================== 引脚配置 ==================== */
// 完全不用CubeMX的引脚配置，全部软件接管
#define OLED_W_SCL(x)  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, (GPIO_PinState)(x))
#define OLED_W_SDA(x)  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, (GPIO_PinState)(x))

/* ==================== 引脚初始化 ==================== */
void OLED_I2C_Init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Pin   = GPIO_PIN_8;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    GPIO_InitStruct.Pin   = GPIO_PIN_9;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    OLED_W_SCL(1);
    OLED_W_SDA(1);
}

/* ==================== 起止信号 ==================== */
void OLED_I2C_Start(void)
{
    OLED_W_SDA(1);
    OLED_W_SCL(1);
    OLED_W_SDA(0);
    OLED_W_SCL(0);
}

void OLED_I2C_Stop(void)
{
    OLED_W_SDA(0);
    OLED_W_SCL(1);
    OLED_W_SDA(1);
}

/* ==================== 发送一个字节 ==================== */
void OLED_I2C_SendByte(uint8_t Byte)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        OLED_W_SDA(!!(Byte & (0x80 >> i)));
        OLED_W_SCL(1);
        OLED_W_SCL(0);
    }
    OLED_W_SCL(1);   // 额外时钟，不处理应答
    OLED_W_SCL(0);
}

/* ==================== 写命令 / 写数据 ==================== */
void OLED_WriteCommand(uint8_t Command)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(0x78);      // 从机地址(7位0x3C << 1)
    OLED_I2C_SendByte(0x00);      // 控制字节：后续为命令
    OLED_I2C_SendByte(Command);
    OLED_I2C_Stop();
}

void OLED_WriteData(uint8_t Data)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(0x78);      // 从机地址
    OLED_I2C_SendByte(0x40);      // 控制字节：后续为数据
    OLED_I2C_SendByte(Data);
    OLED_I2C_Stop();
}

/* ==================== 设置光标 ==================== */
void OLED_SetCursor(uint8_t Y, uint8_t X)
{
    OLED_WriteCommand(0xB0 | Y);                     // 页地址 0~7
    OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));     // 列地址高4位
    OLED_WriteCommand(0x00 | (X & 0x0F));            // 列地址低4位
}

/* ==================== 清屏 ==================== */
void OLED_Clear(void)
{
    uint8_t i, j;
    for (j = 0; j < 8; j++)
    {
        OLED_SetCursor(j, 0);
        for (i = 0; i < 128; i++)
        {
            OLED_WriteData(0x00);
        }
    }
}

/* ==================== 显示一个字符(8x16) ==================== */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
    uint8_t i;
    OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i]);
    }
    OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);
    }
}

/* ==================== 显示字符串 ==================== */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++)
    {
        OLED_ShowChar(Line, Column + i, String[i]);
    }
}

/* ==================== 次方函数 ==================== */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--)
    {
        Result *= X;
    }
    return Result;
}

/* ==================== 显示十进制无符号数 ==================== */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i,
            Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

/* ==================== 显示十进制带符号数 ==================== */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
    uint8_t i;
    uint32_t Number1;
    if (Number >= 0)
    {
        OLED_ShowChar(Line, Column, '+');
        Number1 = (uint32_t)Number;
    }
    else
    {
        OLED_ShowChar(Line, Column, '-');
        Number1 = (uint32_t)(-Number);
    }
    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i + 1,
            Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

/* ==================== 显示十六进制数 ==================== */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i, SingleNumber;
    for (i = 0; i < Length; i++)
    {
        SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
        if (SingleNumber < 10)
        {
            OLED_ShowChar(Line, Column + i, SingleNumber + '0');
        }
        else
        {
            OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A');
        }
    }
}

/* ==================== 显示二进制数 ==================== */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i,
            Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
    }
}

/* ==================== OLED初始化 ==================== */
void OLED_Init(void)
{
    OLED_I2C_Init();

    HAL_Delay(200);      // 上电稳定延时，替代原双层for

    OLED_WriteCommand(0xAE);   // 关闭显示

    OLED_WriteCommand(0xD5);   // 设置显示时钟分频比/振荡器频率
    OLED_WriteCommand(0x80);

    OLED_WriteCommand(0xA8);   // 设置多路复用率
    OLED_WriteCommand(0x3F);

    OLED_WriteCommand(0xD3);   // 设置显示偏移
    OLED_WriteCommand(0x00);

    OLED_WriteCommand(0x40);   // 设置显示开始行

    OLED_WriteCommand(0xA1);   // 左右方向：0xA1正常 0xA0反置
    OLED_WriteCommand(0xC8);   // 上下方向：0xC8正常 0xC0反置

    OLED_WriteCommand(0xDA);   // COM引脚硬件配置
    OLED_WriteCommand(0x12);

    OLED_WriteCommand(0x81);   // 对比度
    OLED_WriteCommand(0xCF);

    OLED_WriteCommand(0xD9);   // 预充电周期
    OLED_WriteCommand(0xF1);

    OLED_WriteCommand(0xDB);   // VCOMH取消选择级别
    OLED_WriteCommand(0x30);

    OLED_WriteCommand(0xA4);   // 整个显示打开/关闭
    OLED_WriteCommand(0xA6);   // 正常/倒转显示

    OLED_WriteCommand(0x8D);   // 电荷泵
    OLED_WriteCommand(0x14);

    OLED_WriteCommand(0xAF);   // 开启显示

    OLED_Clear();
}

