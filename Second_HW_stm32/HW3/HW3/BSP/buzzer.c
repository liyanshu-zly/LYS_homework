#include "buzzer.h"
#include "tim.h"

/*********************************************************************
 * 蜂鸣器驱动实现
 *
 *   TIM2_CH2 -> PA1
 *   72MHz / (71+1) / (999+1) = 1kHz 方波，50% 占空比
 *   更新中断 1kHz（每 1ms 一次），用来给"响 Ms 毫秒"倒计时
 *********************************************************************/

#define BUZZER_CCR_ON    500     /* 50% 占空比：方波最响 */
#define BUZZER_CCR_OFF   0       /* 占空比 0：不发声 */

static volatile uint16_t beep_left = 0;   /* 还要响多少 ms */
static volatile uint8_t  beep_on   = 0;

void Buzzer_Init(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);   /* 使能计数器和 CH2 输出 */
    Buzzer_Off();
    __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_UPDATE); /* 打开更新中断（1kHz） */
}

void Buzzer_Beep(uint16_t Ms)
{
    if (Ms == 0) return;

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, BUZZER_CCR_ON);
    beep_on   = 1;
    beep_left = Ms;
}

void Buzzer_Off(void)
{
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, BUZZER_CCR_OFF);
    beep_on   = 0;
    beep_left = 0;
}

uint8_t Buzzer_IsOn(void)
{
    return beep_on;
}

/* 每 1ms 进来一次（TIM2 更新中断） */
void Buzzer_Tick(void)
{
    if (beep_left)
    {
        beep_left--;
        if (beep_left == 0)
        {
            Buzzer_Off();                   /* 时间到，静音 */
        }
    }
}
