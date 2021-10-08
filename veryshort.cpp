/*
 * File: veryshort.cpp
 *
 * Very short time delay
 *
 */

#include "userdelfx.h"
#include "delayline.hpp"

#define BUF_LEN 257
#define BUF_OFFSET 22
#define MAX_DEPTH 0.99f

static dsp::DelayLine s_delay_r;
static __sdram float s_delay_ram_r[BUF_LEN];

static dsp::DelayLine s_delay_l;
static __sdram float s_delay_ram_l[BUF_LEN];

static float s_depth;
static uint8_t s_time;

void DELFX_INIT(uint32_t platform, uint32_t api)
{
    s_delay_r.setMemory(s_delay_ram_r, BUF_LEN);
    s_delay_l.setMemory(s_delay_ram_l, BUF_LEN);
    s_depth = 0.f;
    s_time = 0;
}

void DELFX_PROCESS(float *main_xn, 
                   uint32_t frames)
{
    float * __restrict my = main_xn;
    const float * my_e = my + 2 * frames;

    float r;
    float dry;
    for (; my != my_e; ) {
        dry = *my;
        r = s_depth * s_delay_l.readFrac(1.0 * s_time);
        s_delay_l.write(dry + r);
        *(my++) = dry + r;

        dry = *my;
        r = s_depth * s_delay_r.readFrac(1.0 * s_time);
        s_delay_r.write(dry + r);
        *(my++) = dry + r;
    }
}


void DELFX_PARAM(uint8_t index, int32_t value)
{
    const float valf = q31_to_f32(value);
    switch (index) {
    case k_user_delfx_param_time:
        s_time = BUF_OFFSET + clip01f(valf * 0.999) * (BUF_LEN - BUF_OFFSET);
        break;
    case k_user_delfx_param_depth:
        s_depth = valf * MAX_DEPTH;
        break;
    case k_user_delfx_param_shift_depth:
        break;
    default:
        break;
  }
}
