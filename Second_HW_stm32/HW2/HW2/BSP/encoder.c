#include "encoder.h"

/*********************************************************************
 * 旋转编码器驱动（GPIO 外部中断 + 正交解码）
 *
 * 思路：
 *   1. A、B 两相都配置成双边沿外部中断；
 *   2. 中断里读回 (A<<1)|B 的四状态，用状态表算出 +1/-1/0；
 *      机械抖动产生的非法跳变在表里是 0，天然起到消抖作用；
 *   3. 按键单独走 EXTI0 下降沿，加 20ms 时间消抖。
 *********************************************************************/

static volatile int32_t  enc_step      = 0;   /* 累计边沿数 */
static volatile uint32_t enc_key_count = 0;   /* 按键次数 */
static volatile uint8_t  enc_last      = 0;   /* 上一次的 (A<<1)|B */
static volatile uint32_t enc_key_tick  = 0;   /* 按键消抖时间戳 */

/* 正交状态表：索引 = (上次状态 << 2) | 本次状态
   0 = 无效/抖动，+1 = 正转，-1 = 反转 */
static const int8_t enc_table[16] =
{
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
};

static uint8_t Encoder_ReadState(void)
{
    uint8_t a = (HAL_GPIO_ReadPin(ENC_A_GPIO_Port, ENC_A_Pin) == GPIO_PIN_SET) ? 1 : 0;
    uint8_t b = (HAL_GPIO_ReadPin(ENC_B_GPIO_Port, ENC_B_Pin) == GPIO_PIN_SET) ? 1 : 0;
    return (uint8_t)((a << 1) | b);
}

void Encoder_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* A、B 两相：上拉输入 + 双边沿中断 */
    GPIO_InitStruct.Pin   = ENC_A_Pin | ENC_B_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* 按键：上拉输入 + 下降沿中断（按下接地） */
    GPIO_InitStruct.Pin  = ENC_KEY_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(ENC_KEY_GPIO_Port, &GPIO_InitStruct);

    enc_step      = 0;
    enc_key_count = 0;
    enc_last      = Encoder_ReadState();

    /* EXTI9 和 EXTI8 共用 EXTI9_5 中断，EXTI0 单独一个 */
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
    HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

/* HAL 的 EXTI 统一回调，所有 EXTI 中断都会走到这里 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == ENC_A_Pin || GPIO_Pin == ENC_B_Pin)
    {
        uint8_t now = Encoder_ReadState();
        int8_t  delta = enc_table[((enc_last & 0x03) << 2) | (now & 0x03)];

        enc_step += delta;
        enc_last  = now;
    }
    else if (GPIO_Pin == ENC_KEY_Pin)
    {
        uint32_t tick = HAL_GetTick();

        if ((tick - enc_key_tick) > 20)   /* 20ms 消抖 */
        {
            enc_key_tick  = tick;
            enc_key_count++;
        }
    }
}

int32_t Encoder_GetStep(void)
{
    return enc_step;
}

void Encoder_ResetStep(void)
{
    enc_step = 0;
}

uint32_t Encoder_GetKeyCount(void)
{
    return enc_key_count;
}

void Encoder_ResetKeyCount(void)
{
    enc_key_count = 0;
}
