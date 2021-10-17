/*
 * File: veryshort.cpp
 *
 * Very short time delay
 *
 */

#include "userdelfx.h"
#include "delayline.hpp"
#include "float_math.h"

#define BUF_LEN 2401
#define MAX_DELAY 2400
#define MIN_DELAY 24
#define MAX_DEPTH 0.95f

static dsp::DelayLine s_delay_r;
static __sdram float s_delay_ram_r[BUF_LEN];

static dsp::DelayLine s_delay_l;
static __sdram float s_delay_ram_l[BUF_LEN];

// depth between -MAX_DEPTH and MAX_DEPTH
static float s_depth;
// time between MIN_DELAY and MAX_DELAY
static float s_time;
static float s_len_z;

void DELFX_INIT(uint32_t platform, uint32_t api)
{
    s_delay_r.setMemory(s_delay_ram_r, BUF_LEN);
    s_delay_l.setMemory(s_delay_ram_l, BUF_LEN);
    s_len_z = 1.f;
    s_depth = 0.f;
    s_time = 0;
}

void DELFX_PROCESS(float *main_xn, 
                   uint32_t frames)
{
    float * __restrict my = main_xn;
    const float * my_e = my + 2 * frames;

    const float len = s_time;
    float len_z = s_len_z;

    float r;
    float dry;
    for (; my != my_e; ) {
        len_z = linintf(0.001f, len_z, len);
        dry = *my;
        r = s_depth * s_delay_l.readFrac(len_z);
        s_delay_l.write(dry + r);
        *(my++) = dry + r;

        dry = *my;
        r = s_depth * s_delay_r.readFrac(len_z);
        s_delay_r.write(dry + r);
        *(my++) = dry + r;
    }
    s_len_z = len_z;
}


void DELFX_PARAM(uint8_t index, int32_t value)
{
    const float valf = q31_to_f32(value);
    switch (index) {
    case k_user_delfx_param_time:
        s_time = MIN_DELAY +  valf * valf * (MAX_DELAY - MIN_DELAY);
        break;
    case k_user_delfx_param_depth:
        s_depth = valf * MAX_DEPTH * 2 - MAX_DEPTH;
        break;
    case k_user_delfx_param_shift_depth:
        break;
    default:
        break;
  }
}
