#include "comm.h"
#include "usart.h"

/*********************************************************************
 * 串口命令解析实现（USART2：PA2 = TX，PA3 = RX，115200 8N1）
 *
 *   逐字节中断接收，收到 '\r' 或 '\n' 认为一帧结束。
 *   帧内容必须是纯数字且在 0~180 之间，否则视为无效帧丢弃：
 *   无效帧不改角度、不响蜂鸣器。
 *********************************************************************/

#define CMD_BUF_LEN   8      /* 最长 3 位数 + 余量，超长帧直接丢弃 */

static uint8_t rx_byte;            /* 单字节接收缓存（HAL 要求传地址） */
static char    cmd_buf[CMD_BUF_LEN];
static uint8_t cmd_len = 0;

static volatile uint8_t target_ready = 0;
static volatile int32_t target_angle = 0;

/* 解析缓冲区里的一帧 */
static void Comm_ParseFrame(void)
{
    int32_t value = 0;
    uint8_t i;

    if (cmd_len == 0) return;

    for (i = 0; i < cmd_len; i++)
    {
        if (cmd_buf[i] < '0' || cmd_buf[i] > '9')
        {
            return;                       /* 出现非数字，整帧作废 */
        }
        value = value * 10 + (cmd_buf[i] - '0');
        if (value > 999) return;          /* 数字太长，作废 */
    }

    if (value > 180) return;              /* 超出舵机量程，作废 */

    target_angle = value;
    target_ready = 1;                     /* 交给主循环处理 */
}

void Comm_Init(void)
{
    cmd_len      = 0;
    target_ready = 0;

    /* 开第一个字节的中断接收，之后在回调里循环重开 */
    HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
}

/* 每收到 1 个字节进一次：USART2_IRQHandler -> HAL_UART_IRQHandler -> 这里 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    char c;

    if (huart->Instance != USART2) return;

    c = (char)rx_byte;

    if (c == '\r' || c == '\n')
    {
        Comm_ParseFrame();                /* 一帧结束，尝试解析 */
        cmd_len = 0;
    }
    else if (c == ' ' || c == '\t')
    {
        /* 忽略空白字符 */
    }
    else
    {
        if (cmd_len < CMD_BUF_LEN)
        {
            cmd_buf[cmd_len++] = c;
        }
        else
        {
            cmd_len = 0;                  /* 超长帧，丢弃重来 */
        }
    }

    /* HAL 的 IT 模式收完一个字节会关掉接收，必须重新开 */
    HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
}

uint8_t Comm_PopTarget(int32_t *Angle)
{
    if (!target_ready) return 0;

    *Angle       = target_angle;
    target_ready = 0;
    return 1;
}
