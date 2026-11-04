#include "buzzer.h"
#include "tim.h"

/*********************************************************************
 * 蜂鸣器驱动实现
 *
 *   TIM2_CH2 -> PA1
 *   72MHz / (71+1) / (999+1) = 1kHz 方波，50% 占空比
 *   更新中断 1kHz（每 1ms 一次），用软件计数做 0.2s / 1s 的节奏
 *********************************************************************/

#define BUZZER_CCR_ON    500     /* 50% 占空比：方波最响 */
#define BUZZER_CCR_OFF   0       /* 占空比 0：不发声 */

#define BEEP_ON_MS       200     /* 每次响 0.2s */
#define BEEP_PERIOD_MS  1000     /* 每 1s 一个循环 */

static volatile uint16_t buzzer_ms = 0;   /* 1ms 计数 */
static volatile uint8_t  buzzer_sw = 0;   /* 0 = 静音，1 = 正在响 */

void Buzzer_Init(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);   /* 使能计数器和 CH2 输出 */
    Buzzer_Off();
    buzzer_ms = 0;
    __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_UPDATE); /* 打开更新中断（1kHz） */
}

void Buzzer_On(void)
{
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, BUZZER_CCR_ON);
    buzzer_sw = 1;
}

void Buzzer_Off(void)
{
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, BUZZER_CCR_OFF);
    buzzer_sw = 0;
}

uint8_t Buzzer_IsOn(void)
{
    return buzzer_sw;
}

/* 每 1ms 进来一次（TIM2 更新中断） */
void Buzzer_Tick(void)
{
    buzzer_ms++;

    if (buzzer_ms <= BEEP_ON_MS)
    {
        if (!buzzer_sw) Buzzer_On();        /* 0 ~ 200ms：响 */
    }
    else
    {
        if (buzzer_sw)  Buzzer_Off();       /* 200 ~ 1000ms：停 */
    }

    if (buzzer_ms >= BEEP_PERIOD_MS)
    {
        buzzer_ms = 0;                      /* 满 1s 重新开始 */
    }
}
