#pragma once
/*
 *  File: delay.h
 *
 */

#include <atomic>
#include <cstddef>
#include <cstdint>

#include <arm_neon.h>

constexpr size_t samplerate = 48000;
constexpr size_t samples_1ms = (samplerate / 1000);

constexpr size_t param_time = 0;
constexpr size_t param_depth = 1;

// delay line length = 2 channels * 32768 samples (= 682 msec)
constexpr size_t delay_size = 1 << 16;
constexpr size_t delay_mask = delay_size - 1;

// linear interpolation
static inline __attribute__((optimize("Ofast"), always_inline))
float linintf(const float fr, const float x0, const float x1) {
  return x0 + fr * (x1 - x0);
}

class Delay {
public:

    Delay(void) {};
    virtual ~Delay(void) {};

    inline int8_t Init(const unit_runtime_desc_t * desc) {

	if (desc->samplerate != 48000) {
	    return k_unit_err_samplerate;
	}

	if (desc->input_channels != 2 || desc->output_channels != 2) {
	    return k_unit_err_geometry;
	}

	delay_clear();

	return k_unit_err_none;
    }

    inline void Teardown() {
    }

    inline void Reset() {
	delay_clear();
    }

    inline void Resume() {
	delay_clear();
    }

    inline void Suspend() {
    }

    fast_inline void Process(const float * in, float * out, size_t frames) {
	const float32x2_t * __restrict in_p = (float32x2_t *) in;
	float * __restrict out_p = out;
	const float * out_e = out_p + (frames << 1);

	for (; out_p != out_e; in_p++, out_p += 2) {
	    time_z_ = linintf(0.001f, time_z_, time_);
	    float32x2_t sig = delay_read(time_z_ * samples_1ms);
	    sig = vadd_f32(*in_p, vmul_n_f32(sig, depth_));
	    delay_write(sig);
	    vst1_f32(out_p, sig);
	}
    }

    inline void setParameter(uint8_t index, int32_t value) {
	(void)value;
	switch (index) {
	case param_time:
	    time_i_ = value;
	    time_ = value * .1f;
	    break;
	case param_depth:
	    depth_i_ = value;
	    depth_ = value * 0.005f;
	    break;
	default:
	    break;
	}
    }

    inline int32_t getParameterValue(uint8_t index) const {
	switch (index) {
	case param_time:
	    return time_i_;
	case param_depth:
	    return depth_i_;
	default:
	    break;
	}
	return 0;
    }

    inline const char * getParameterStrValue(uint8_t index, int32_t value) const {
	(void)value;
	switch (index) {
	default:
	    break;
	}
	return nullptr;
    }

    inline const uint8_t * getParameterBmpValue(uint8_t index,
						int32_t value) const {
	(void)value;
	switch (index) {
	default:
	    break;
	}
	return nullptr;
    }

    inline void LoadPreset(uint8_t idx) { (void)idx; }

    inline uint8_t getPresetIndex() const { return 0; }

    /*=======================================================================*/
    /* Static Members. */
    /*=======================================================================*/

    static inline const char * getPresetName(uint8_t idx) {
	(void)idx;
	return nullptr;
    }

private:
    /*=======================================================================*/
    /* Private Member Variables. */
    /*=======================================================================*/

    std::atomic_uint_fast32_t flags_;

    float delay_line_[delay_size] __attribute__((aligned(16)));
    int32_t delay_pos_;

    float time_;     // in milliseconds, 0.5ms steps
    float time_z_;
    float depth_;    // -0.99 < depth_ < 0.99, 0.005 steps
    int32_t time_i_; // time_ * 10
    int32_t depth_i_;// depth_ * 200

    /*=======================================================================*/
    /* Private Methods. */
    /*=======================================================================*/

    void delay_clear() {
	float *end = &delay_line_[delay_size];
	for(float *p = delay_line_; p != end; p++) {
	    *p = 0.f;
	}
	delay_pos_ = 0;
    }

    void delay_write(float32x2_t sig) {
	vst1_f32(&delay_line_[delay_pos_], sig);
	delay_pos_ = (delay_pos_ + 2) & delay_mask;
    }

    float32x2_t delay_read(float pos) {
	// Note: this code doesn't interpolate frac part of pos
	int32_t p = int(pos);
	p = (delay_pos_ - (p << 1)) & delay_mask;
	return vld1_f32(&delay_line_[p]);
    }

    /*=======================================================================*/
    /* Constants. */
    /*=======================================================================*/

};
