/*
 * File: veryshort.cpp
 *
 * Very short time delay
 *
 */

//#include "userdelfx.h"
#include "unit_delfx.h"
#include "dsp/delayline.hpp"
#include "utils/float_math.h"
#include "utils/buffer_ops.h" // for buf_clr_f32()
#include "utils/int_math.h"   // for clipminmaxi32()

#define BUF_LEN 2401
#define MAX_DELAY 2400
#define MIN_DELAY 24
#define MAX_DEPTH 0.98f

static dsp::DelayLine s_delay_r;
static float *s_delay_ram_r;

static dsp::DelayLine s_delay_l;
static float *s_delay_ram_l;

// depth between -MAX_DEPTH and MAX_DEPTH
static float s_depth;
// time between MIN_DELAY and MAX_DELAY
static float s_time;
static float s_len_z;
static float s_mix = 0.f;

unit_runtime_desc_t s_desc;
int32_t p_[11];

enum {
    TIME = 0U,
    DEPTH,
    MIX,
    NUM_PARAMS
};

inline float *sdram_alloc_f32(size_t bufsize) {
    float *m = (float *)s_desc.hooks.sdram_alloc(bufsize * sizeof(float));
    if (m) {
        buf_clr_f32(m, bufsize);
    }
    return m;
}

//void DELFX_INIT(uint32_t platform, uint32_t api)
//{
__unit_callback int8_t unit_init(const unit_runtime_desc_t * desc) {
    if (!desc)
        return k_unit_err_undef;
    
    s_desc = *desc;

    if (desc->target != unit_header.target)
        return k_unit_err_target;
    
    if (!UNIT_API_IS_COMPAT(desc->api))
        return k_unit_err_api_version;
    
    if (desc->samplerate != 48000)
        return k_unit_err_samplerate;

    if (desc->input_channels != 2 || desc->output_channels != 2)
        return k_unit_err_geometry;

    // If SDRAM buffers are required they must be allocated here
    if (!desc->hooks.sdram_alloc)
        return k_unit_err_memory;

/*
    float *m = (float *)desc->hooks.sdram_alloc(BUF_LEN * sizeof(float));
    if (!m)
        return k_unit_err_memory;

    buf_clr_f32(m, BUF_LEN);

*/
    s_delay_ram_r = sdram_alloc_f32(BUF_LEN);
    s_delay_r.setMemory(s_delay_ram_r, BUF_LEN);
/*
    m = (float *)desc->hooks.sdram_alloc(BUF_LEN * sizeof(float));
    if (!m)
        return k_unit_err_memory;

    buf_clr_f32(m, BUF_LEN);

*/
    s_delay_ram_l = sdram_alloc_f32(BUF_LEN);
    s_delay_l.setMemory(s_delay_ram_l, BUF_LEN);
    

    s_len_z = 1.f;
    s_depth = 0.f;
    s_time = 0;

    return k_unit_err_none;
}

//void DELFX_PROCESS(float *main_xn, 
//                   uint32_t frames)
__unit_callback void unit_render(const float * in, float * out, uint32_t frames)
{
//    float * __restrict my = main_xn;
    const float * __restrict in_p = in;
    float * __restrict my = out;
    const float * my_e = my + 2 * frames;

    const float len = s_time;
    float len_z = s_len_z;

    float r;
    float dry;
    for (; my != my_e; ) {
        len_z = linintf(0.001f, len_z, len);
        dry = *in_p++;
        r = s_depth * s_delay_l.readFrac(len_z);
        s_delay_l.write(dry + r);
        *(my++) = (s_mix < 0 ? dry : dry * (1 - s_mix)) + \
            (s_mix > 0 ? r : r * (1 + s_mix));

        dry = *in_p++;
        r = s_depth * s_delay_r.readFrac(len_z);
        s_delay_r.write(dry + r);
//        *(my++) = dry + r;
        *(my++) = (s_mix < 0 ? dry : dry * (1 - s_mix)) + \
            (s_mix > 0 ? r : r * (1 + s_mix));
    }
    s_len_z = len_z;
}


// void DELFX_PARAM(uint8_t index, int32_t value)
//{
__unit_callback void unit_set_param_value(uint8_t id, int32_t value) {
//    const float valf = q31_to_f32(value);
    float valf;
    switch (id) {
    case TIME:
        value = clipminmaxi32(0, value, 1023);
        valf = param_10bit_to_f32(value);
        s_time = MIN_DELAY +  valf * valf * (MAX_DELAY - MIN_DELAY);
        break;
    case DEPTH:
        value = clipminmaxi32(0, value, 1023);
        valf = param_10bit_to_f32(value);
        s_depth = valf * MAX_DEPTH * 2 - MAX_DEPTH;
        break;
    case MIX:
        value = clipminmaxi32(-100, value, 100);
        s_mix = value / 100.f;
        break;
    default:
        break;
  }
}

__unit_callback void unit_teardown() {
}

__unit_callback void unit_reset() {
}

__unit_callback void unit_resume() {
}

__unit_callback void unit_suspend() {
}

__unit_callback int32_t unit_get_param_value(uint8_t id) {
    return p_[id];
}

__unit_callback const char * unit_get_param_str_value(uint8_t id, int32_t value) {
    return nullptr;
}

__unit_callback void unit_set_tempo(uint32_t tempo) {
}

__unit_callback void unit_tempo_4ppqn_tick(uint32_t counter) {
}
