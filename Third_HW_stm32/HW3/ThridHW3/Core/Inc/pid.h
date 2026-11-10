#ifndef __PID_H
#define __PID_H

/*
 * 通用位置式 PID（条件积分抗饱和 + 微分先行）
 *
 * 单位约定（本作业）：
 *   setpoint / meas : rpm
 *   out             : PWM 比较值（0 ~ PWM_MAX）
 *   dt              : 采样周期，单位秒
 */

typedef struct
{
    float kp;
    float ki;
    float kd;
    float out_min;
    float out_max;
    float integ;        /* 积分项累计值（已经是“比较值”量纲） */
    float meas_prev;    /* 上一次测量值，用于微分先行 */
    float out;          /* 上一次输出 */
    unsigned char first;
} PID_t;

void  PID_Init(PID_t *pid, float kp, float ki, float kd,
               float out_min, float out_max);
void  PID_Reset(PID_t *pid);
float PID_Update(PID_t *pid, float setpoint, float meas, float dt);

#endif
