#include "pid.h"

static float PID_Clamp(float v, float lo, float hi)
{
    if (v < lo)
    {
        return lo;
    }
    if (v > hi)
    {
        return hi;
    }
    return v;
}

void PID_Init(PID_t *pid, float kp, float ki, float kd,
              float out_min, float out_max)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->out_min = out_min;
    pid->out_max = out_max;
    PID_Reset(pid);
}

void PID_Reset(PID_t *pid)
{
    pid->integ = 0.0f;
    pid->meas_prev = 0.0f;
    pid->out = 0.0f;
    pid->first = 1U;
}

float PID_Update(PID_t *pid, float setpoint, float meas, float dt)
{
    float err;
    float p;
    float d;
    float i;
    float u;

    if (dt <= 0.0f)
    {
        return pid->out;
    }

    err = setpoint - meas;

    /* 比例项 */
    p = pid->kp * err;

    /* 微分先行：对测量值求导，设定值阶跃时不会产生微分冲击 */
    if (pid->first != 0U)
    {
        d = 0.0f;
        pid->first = 0U;
    }
    else
    {
        d = -pid->kd * (meas - pid->meas_prev) / dt;
    }
    pid->meas_prev = meas;

    /* 积分项（先算候选值，饱和时再决定是否接受，即条件积分） */
    i = pid->integ + pid->ki * err * dt;
    i = PID_Clamp(i, pid->out_min, pid->out_max);

    u = p + i + d;

    if (u > pid->out_max)
    {
        u = pid->out_max;
        /* 已经顶到上限：只有误差反向（会让输出回落）时才继续积分 */
        if (err < 0.0f)
        {
            pid->integ = i;
        }
    }
    else if (u < pid->out_min)
    {
        u = pid->out_min;
        if (err > 0.0f)
        {
            pid->integ = i;
        }
    }
    else
    {
        pid->integ = i;
    }

    pid->out = u;
    return u;
}
