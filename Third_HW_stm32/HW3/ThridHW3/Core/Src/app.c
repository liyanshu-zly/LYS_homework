#include "main.h"
#include "tim.h"
#include "gpio.h"
#include "OLED.h"
#include "pid.h"
#include "app.h"

/* =====================================================================
 *  HW3：TB6612 + 310 电机 + 电机编码器 + 旋钮编码器 + OLED + PID 闭环
 *
 *  引脚：
 *    TIM2_CH1 / PA0  -> TB6612 PWMA   （20 kHz PWM，CCR 0~3600）
 *    PB0 / PB1       -> AIN1 / AIN2   （方向）
 *    TIM3 / PA6 PA7  -> 旋钮编码器    （调目标转速）
 *    TIM1 / PA8 PA9  -> 电机编码器    （测实际转速）
 *    PB8 / PB9       -> OLED 软件 I2C （不变）
 *    TIM4            -> 10 ms 控制中断（测速 + PID + 更新 PWM）
 *
 *  主循环只刷 OLED，控制全部在 TIM4 中断里完成。
 * ===================================================================== */

/* ---------------- 可调参数 ---------------- */
#define CTRL_PERIOD_MS    10.0f     /* TIM4 中断周期，必须和 CubeMX 里一致 */
#define PWM_MAX           3600      /* TIM2: PSC=0, ARR=3599 -> 20 kHz */

/* 电机输出轴转一圈对应的“4 倍频计数”，必须实测校准！ */
#define MOTOR_CPR         2112.0f

/* 电机编码器计数方向：正转时 ACT 为负就把这里改成 (-1) */
#define MOTOR_ENC_SIGN    (+1)

#define SPEED_LPF_K       0.45f     /* 转速一阶低通系数 */
#define SPEED_DEADBAND    0.5f      /* 小于 0.5 rpm 当 0 */

#define KNOB_DIV          4         /* 旋钮 4 个计数 = 1 格 */
#define TGT_STEP_RPM      10.0f     /* 每格改 10 rpm */
#define TGT_LIMIT_RPM     300.0f    /* 目标转速上限，按电机实际能力改 */
#define TGT_RAMP_RPS      400.0f    /* 目标斜坡：每秒最多变化多少 rpm */

#define PWM_DEADBAND      25        /* 输出死区，防止零速附近嗡嗡响 */

#define KP_DEFAULT        15.0f     /* 单位：比较值 / rpm */
#define KI_DEFAULT        25.0f     /* 单位：比较值 / (rpm * s) */
#define KD_DEFAULT        0.0f

/* 调试开关：1 = 开环固定 30% 占空比（确认转向 / 编码器符号） */
#define TEST_OPEN_LOOP    0
/* 调试开关：1 = 第三行显示累计计数（校准 MOTOR_CPR） */
#define DEBUG_RAW_COUNTS  0

/* ---------------- 内部状态 ---------------- */
static PID_t pid;

static int16_t enc_last;        /* TIM1 上次计数 */
static int16_t knob_last;       /* TIM3 上次计数 */
static int32_t knob_frac;       /* 旋钮除不尽的余数 */
static volatile int32_t enc_total;    /* 电机编码器累计计数（校准用） */
static volatile int32_t knob_pos;     /* 旋钮编码器的值（格数） */
static volatile float   speed_rpm;    /* 实测转速 */
static volatile float   tgt_cmd;      /* 旋钮给出的目标转速 */
static volatile float   tgt_ramp;     /* 斜坡后的目标转速 */
static volatile float   duty_cmd;     /* 当前 PWM 比较值 */

static uint32_t ui_tick;
static int32_t  ui_knob;
static int32_t  ui_tgt;
static int32_t  ui_act;
static int32_t  ui_pwm;

static void Motor_Output(int32_t cmd);
static void UI_Init(void);
static void UI_Update(void);
static void UI_ShowUInt(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
static void UI_ShowInt(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);

/* ---------------- 电机输出（只有正转，停车时滑行） ---------------- */
static void Motor_Output(int32_t cmd)
{
    uint32_t mag;

    if (cmd > 0)
    {
        /* 正转：AIN1=1, AIN2=0 */
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
        mag = (uint32_t)cmd;
    }
    else
    {
        /* 停止：两路都低，电机自由滑行 */
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
        mag = 0U;
    }

    if (mag > (uint32_t)PWM_MAX)
    {
        mag = (uint32_t)PWM_MAX;
    }

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, mag);
}

/* ---------------- 数字显示（右对齐，高位补空格） ---------------- */
static void UI_ShowUInt(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    char buf[10];
    uint8_t i;

    if (Length > 8U)
    {
        Length = 8U;
    }

    for (i = 0U; i < Length; i++)
    {
        buf[i] = ' ';
    }
    buf[Length] = '\0';

    i = Length;
    while (i > 0U)
    {
        i--;
        buf[i] = (char)('0' + (Number % 10U));
        Number /= 10U;
        if (Number == 0U)
        {
            break;
        }
    }

    OLED_ShowString(Line, Column, buf);
}

static void UI_ShowInt(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
    char buf[10];
    uint8_t i;
    uint32_t mag;

    if (Length > 7U)
    {
        Length = 7U;
    }

    if (Number < 0)
    {
        buf[0] = '-';
        mag = (uint32_t)(-(Number + 1)) + 1U;   /* 防止 INT32_MIN 取反溢出 */
    }
    else
    {
        buf[0] = '+';
        mag = (uint32_t)Number;
    }

    for (i = 1U; i <= Length; i++)
    {
        buf[i] = ' ';
    }
    buf[Length + 1U] = '\0';

    i = (uint8_t)(Length + 1U);
    while (i > 1U)
    {
        i--;
        buf[i] = (char)('0' + (mag % 10U));
        mag /= 10U;
        if (mag == 0U)
        {
            break;
        }
    }

    OLED_ShowString(Line, Column, buf);
}

static void UI_Init(void)
{
    OLED_ShowString(1, 1, "ENC:");
    OLED_ShowString(2, 1, "TGT:");
    OLED_ShowString(3, 1, "ACT:");
    OLED_ShowString(4, 1, "PWM:");

    OLED_ShowString(2, 10, "RPM");
    OLED_ShowString(3, 10, "RPM");
    OLED_ShowString(4, 10, "PID");

    ui_knob = -99999;
    ui_tgt  = -99999;
    ui_act  = -99999;
    ui_pwm  = -99999;

    UI_Update();
}

static void UI_Update(void)
{
    int32_t iv;

    /* 第 1 行：旋钮编码器的值 */
    iv = knob_pos;
    if (iv != ui_knob)
    {
        ui_knob = iv;
        UI_ShowUInt(1, 5, (uint32_t)iv, 3);
    }

    /* 第 2 行：目标转速（题目要求：目标值显示在第二行） */
    iv = (int32_t)tgt_cmd;
    if (iv != ui_tgt)
    {
        ui_tgt = iv;
        UI_ShowUInt(2, 5, (uint32_t)iv, 3);
    }

    /* 第 3 行：实际转速 */
#if DEBUG_RAW_COUNTS
    iv = enc_total;
    if (iv != ui_act)
    {
        ui_act = iv;
        UI_ShowInt(3, 5, iv, 5);
    }
#else
    iv = (int32_t)speed_rpm;
    if (iv != ui_act)
    {
        ui_act = iv;
        UI_ShowInt(3, 5, iv, 3);
    }
#endif

    /* 第 4 行：PWM 比较值 */
    iv = (int32_t)duty_cmd;
    if (iv != ui_pwm)
    {
        ui_pwm = iv;
        UI_ShowUInt(4, 5, (uint32_t)iv, 4);
    }
}

/* ---------------- 初始化 ---------------- */
void App_Init(void)
{
    Motor_Output(0);

    enc_last = 0;
    knob_last = 0;
    knob_frac = 0;
    enc_total = 0;
    knob_pos = 0;
    speed_rpm = 0.0f;
    tgt_cmd = 0.0f;
    tgt_ramp = 0.0f;
    duty_cmd = 0.0f;

    PID_Init(&pid, KP_DEFAULT, KI_DEFAULT, KD_DEFAULT,
             0.0f, (float)PWM_MAX);

    /* Display first, so a later timer problem cannot hide the UI. */
    OLED_Init();
    OLED_Clear();
    UI_Init();

    (void)HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    (void)HAL_TIM_Encoder_Start(&htim1, TIM_CHANNEL_ALL);   /* 电机编码器 */
    (void)HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);   /* 旋钮编码器 */

    __HAL_TIM_SET_COUNTER(&htim1, 0U);
    __HAL_TIM_SET_COUNTER(&htim3, 0U);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0U);

    ui_tick = HAL_GetTick();

    /* 最后再开 10 ms 中断，保证上面的初值都已就绪 */
    (void)HAL_TIM_Base_Start_IT(&htim4);
}

/* ---------------- 10 ms 控制中断 ---------------- */
void App_TimerISR(TIM_HandleTypeDef *htim)
{
    int16_t now;
    int16_t delta;
    float   raw;
    float   u;
    float   lim;
    float   step;
    int32_t mag;

    if (htim->Instance != TIM4)
    {
        return;
    }

    /* ---- 1. 电机编码器 -> 转速 ---- */
    now = (int16_t)__HAL_TIM_GET_COUNTER(&htim1);
    delta = (int16_t)(now - enc_last);
    enc_last = now;
    enc_total += delta;

    /* delta 个计数 / 10 ms -> rpm：delta * 60000 / (CPR * 10) */
    raw = (float)delta * (float)MOTOR_ENC_SIGN *
          (60000.0f / (MOTOR_CPR * CTRL_PERIOD_MS));

    speed_rpm += SPEED_LPF_K * (raw - speed_rpm);
    if ((speed_rpm < SPEED_DEADBAND) && (speed_rpm > -SPEED_DEADBAND))
    {
        speed_rpm = 0.0f;
    }

    /* ---- 2. 旋钮编码器 -> 目标转速 ---- */
    now = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);
    delta = (int16_t)(now - knob_last);
    knob_last = now;
    knob_frac += (int32_t)delta;

    while (knob_frac >= KNOB_DIV)
    {
        knob_frac -= KNOB_DIV;
        if (tgt_cmd < TGT_LIMIT_RPM)
        {
            tgt_cmd += TGT_STEP_RPM;
            knob_pos++;
        }
    }
    while (knob_frac <= -KNOB_DIV)
    {
        knob_frac += KNOB_DIV;
        if (tgt_cmd > 0.0f)
        {
            tgt_cmd -= TGT_STEP_RPM;
            knob_pos--;
        }
    }

    /* ---- 3. 目标斜坡：防止设定值突变导致电流冲击 / 超调 ---- */
    lim = TGT_RAMP_RPS * (CTRL_PERIOD_MS / 1000.0f);
    step = tgt_cmd - tgt_ramp;
    if (step > lim)
    {
        step = lim;
    }
    if (step < -lim)
    {
        step = -lim;
    }
    tgt_ramp += step;

    if (tgt_cmd <= 0.0f)
    {
        tgt_ramp = 0.0f;
    }

    /* ---- 4. PID ---- */
    if (tgt_ramp < 0.5f)
    {
        PID_Reset(&pid);
        u = 0.0f;
    }
    else
    {
#if TEST_OPEN_LOOP
        u = 0.30f * (float)PWM_MAX;     /* 开环 30%，只用来验证方向 */
#else
        u = PID_Update(&pid, tgt_ramp, speed_rpm,
                       CTRL_PERIOD_MS / 1000.0f);
#endif
    }

    /* ---- 5. 限幅 + 死区 + 输出到 TB6612 ---- */
    mag = (int32_t)u;
    if (mag < 0)
    {
        mag = 0;
    }
    if (mag > (int32_t)PWM_MAX)
    {
        mag = (int32_t)PWM_MAX;
    }
    if (mag < PWM_DEADBAND)
    {
        mag = 0;
    }

    Motor_Output(mag);
    duty_cmd = (float)mag;
}

/* ---------------- 主循环：只刷 OLED ---------------- */
void App_Loop(void)
{
    uint32_t now;

    now = HAL_GetTick();
    if ((now - ui_tick) >= 100U)
    {
        ui_tick = now;
        UI_Update();
    }

    HAL_Delay(1U);
}
