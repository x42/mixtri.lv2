/* Mixer + Trigger LV2
 *
 * Copyright (C) 2013 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <ltc.h>

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "mixtri.h"

#ifndef MIN
#define MIN(A,B) ( (A) < (B) ? (A) : (B) )
#endif
#ifndef MAX
#define MAX(A,B) ( (A) > (B) ? (A) : (B) )
#endif

/******************************************************************************
 * trigger helpers
 */

#define FIRE_TRIGGER (1.0)

static inline bool edge_trigger(const int mode, const float lvl, const float v0, const float v1) {
	if (   (mode&1) /* rising edge*/
			&& (v0 <= lvl) && (v1 > lvl)) return true;
	if (   (mode&2) /* falling edge*/
			&& (v0 >= lvl) && (v1 < lvl)) return true;
	return false;
}

static inline int range_trigger(const float l0, const float l1, const float v0, const float v1) {
	assert (l0 <= l1);
	/* bit 1: falling edge, 2:rising edge  || bit(3) 4: outside */
	// prev signal was outside, new signal inside
	if ((v0 > l1 || v0 < l0) && v1 >= l0 && v1 <= l1) return (v0 > l1) ? 2 : 1;
	// prev signal inside , new signal outside range
	if (v0 <= l1 && v0 >= l0 && (v1 < l0 || v1 > l1)) return (v1 < l0) ? 6 : 5;
	return 0;
}

/******************************************************************************
 * LV2 routines
 */

#define FADE_LEN (64)

typedef struct {
	float buffer[MAXDELAY];
	int c_dly;
	int w_ptr;
	int r_ptr;
} DelayLine;

typedef struct {
	/* LV2 ports */
	float* a_in[4];
	float* a_out[4];

	float* p_gain_in[4];
	float* p_mix[12];
	float* p_delay[7];
	float* p_input[4];
	float* p_trigger_chn;
	float* p_trigger_mode;

	float* p_trigger_edge;
	float* p_trigger_level0;
	float* p_trigger_level1;
	float* p_trigger_time0;
	float* p_trigger_time1;

	double rate;

	/* internal state, delaylines */
	DelayLine dly_i[4];
	DelayLine dly_o[3];

	/* internal state, filters */
	int   mode_input[4];
	float flt_z[4];
	float flt_y[4];
	float flt_alpha;

	float gain_db[4];
	float gain_in[4];

	float amp_z[4];
	float mix_z[12];
	float mix_alpha;

	/* trigger state */
	LTCDecoder *decoder;
	uint64_t monotonic_cnt;
	uint64_t ts_time;
	float ts_prev;
	int ts_hysteresis;

	/* trigger settings cache */
	int   tri_mode_prev;
	float tri_alpha;
	float tri_t0;

} MixTri;

static LV2_Handle
instantiate(
		const LV2_Descriptor*     descriptor,
		double                    rate,
		const char*               bundle_path,
		const LV2_Feature* const* features)
{
	MixTri* self = (MixTri*)calloc(1, sizeof(MixTri));
	if(!self) {
		return NULL;
	}

	self->rate = rate;

	/* low/high pass filter time-constants */
	// 1.0 - e^(-2.0 * π * v / 48000)
	self->mix_alpha = 1.0f - expf(-2.0 * M_PI * 100 / rate); // LPF
	self->flt_alpha = 1.0 - 5.0 / rate; // HPF

	for (uint32_t i = 0; i < 4; ++i) {
		self->mode_input[i] = 0;
		self->flt_z[i] = 0;
		self->flt_y[i] = 0;
		memset(self->dly_i[i].buffer, 0, sizeof(float) * MAXDELAY);
		self->gain_db[i] = 0.0;
		self->gain_in[i] = 1.0;
		self->amp_z[i] = 0;
	}
	for (uint32_t i = 0; i < 3; ++i) {
		memset(self->dly_o[i].buffer, 0, sizeof(float) * MAXDELAY);
	}
	for (uint32_t i = 0; i < 12; ++i) {
		self->mix_z[i] = 0;
	}

	/* trigger settings & state*/
	self->decoder = ltc_decoder_create(rate / 25, 8);
	self->monotonic_cnt = 0;
	self->ts_time = 0;
	self->ts_prev = 0;
	self->ts_hysteresis = 0;

	self->tri_t0 = -1;
	self->tri_alpha = 1.0f;
	self->tri_mode_prev = 0;
	return (LV2_Handle)self;
}

static void
connect_port_mixtri(
		LV2_Handle handle,
		uint32_t   port,
		void*      data)
{
	MixTri* self = (MixTri*)handle;

	switch ((PortIndexMixTri)port) {
		case MIXTRI_TRIG_CHN:
			self->p_trigger_chn = (float*)data;
			break;
		case MIXTRI_TRIG_MODE:
			self->p_trigger_mode = (float*)data;
			break;
		case MIXTRI_TRIG_EDGE:
			self->p_trigger_edge = (float*)data;
			break;
		case MIXTRI_TRIG_LVL0:
			self->p_trigger_level0 = (float*)data;
			break;
		case MIXTRI_TRIG_LVL1:
			self->p_trigger_level1 = (float*)data;
			break;
		case MIXTRI_TRIG_TME0:
			self->p_trigger_time0 = (float*)data;
			break;
		case MIXTRI_TRIG_TME1:
			self->p_trigger_time1 = (float*)data;
			break;
		default:
			if (port >= MIXTRI_AUDIO_IN_0 && port <= MIXTRI_AUDIO_IN_3) {
				self->a_in[port - MIXTRI_AUDIO_IN_0] = (float*)data;
			}
			else if (port >= MIXTRI_AUDIO_OUT_0 && port <= MIXTRI_AUDIO_OUT_T) {
				self->a_out[port - MIXTRI_AUDIO_OUT_0] = (float*)data;
			}
			else if (port >= MIXTRI_MIX_0_0 && port <= MIXTRI_MIX_3_2) {
				self->p_mix[port - MIXTRI_MIX_0_0] = (float*)data;
			}
			else if (port >= MIXTRI_DLY_I_0 && port <= MIXTRI_DLY_O_2) {
				self->p_delay[port - MIXTRI_DLY_I_0] = (float*)data;
			}
			else if (port >= MIXTRI_GAIN_I_0 && port <= MIXTRI_GAIN_I_3) {
				self->p_gain_in[port - MIXTRI_GAIN_I_0] = (float*)data;
			}
			else if (port >= MIXTRI_MOD_I_0 && port <= MIXTRI_MOD_I_3) {
				self->p_input[port - MIXTRI_MOD_I_0] = (float*)data;
			}
			break;
	}
}

static void
run(LV2_Handle handle, uint32_t n_samples)
{
	MixTri* self = (MixTri*)handle;

	/* localize variable into scope */
	float const * const * a_i = (float const * const *) self->a_in;
	float * const * a_o = self->a_out;
	float mix_t[12], mix[12];
	int fade_i[4] = { 0, 0, 0, 0};
	int fade_o[3] = { 0, 0, 0};
	int delay_i[4], delay_o[3];
	float flt_z[4], flt_y[4];
	float amp_in_t[4], amp_in[4];
	int cmode_in[4];
	int pmode_in[4];

	/* trigger settings*/
	int   tt_edgemode = *self->p_trigger_edge;
	float tt_level0   = *self->p_trigger_level0;
	float tt_level1   = *self->p_trigger_level1;
	uint64_t tt_tme0  = *self->p_trigger_time0;
	uint64_t tt_tme1  = *self->p_trigger_time1;

	float ts_prev = self->ts_prev;
	uint64_t ts_time = self->ts_time;
	int ts_hysteresis = self->ts_hysteresis;

	const uint64_t monotonic_cnt = self->monotonic_cnt;
	const float fade_len = (n_samples >= FADE_LEN) ? FADE_LEN : ceilf(n_samples / 2.f);
	const float flt_alpha = self->flt_alpha;
	const float mix_alpha = self->mix_alpha;
	const int trigger_mode = *self->p_trigger_mode;
	const int trigger_chn  = MIN(3, MAX(0, *self->p_trigger_chn));
	float tri_alpha = self->tri_alpha;

	if (trigger_mode != self->tri_mode_prev)  {
		self->tri_mode_prev= trigger_mode;
		ts_time = 0;
		ts_hysteresis = 0;
		switch(trigger_mode) {
			case TRG_PULSETRAIN:
			case TRG_DROPIN:
			case TRG_DROPOUT:
				ts_time = 1e18;
				break;
			default:
				ts_time = 0;
				break;
		}
	}

	if ((trigger_mode == TRG_LPF || trigger_mode == TRG_RMS) && self->tri_t0 != tt_tme0) {
		self->tri_t0 = tt_tme0;
		if (tt_tme0 > self->rate) {
			self->tri_alpha = 1.0f;
		} else {
			if (tt_tme0 < 1) tt_tme0 = 1;
			self->tri_alpha = 1.0f - expf(-2.0 * M_PI * (float)tt_tme0 / self->rate);
		}
		tri_alpha = self->tri_alpha;
	}

	for (uint32_t i = 0; i < 12; ++i) {
		mix_t[i] = *(self->p_mix[i]);
		mix[i] = self->mix_z[i];
	}

	for (uint32_t i = 0; i < 4; ++i) {
		flt_z[i] = self->flt_z[i];
		flt_y[i] = self->flt_y[i];
		cmode_in[i] = *(self->p_input[i]);
		pmode_in[i] = self->mode_input[i];
		amp_in_t[i] = self->gain_in[i];
		amp_in[i] = self->amp_z[i];

		delay_i[i] = *(self->p_delay[i]);
		if (delay_i[i] != self->dly_i[i].c_dly) {
			/* input delay time changed */
			fade_i[i] = fade_len;
		}
		if (self->gain_db[i] != *(self->p_gain_in[i])) {
			/* recalc gain only if changed */
			self->gain_db[i] = *(self->p_gain_in[i]);
			self->gain_in[i] = pow(10, .05 * self->gain_db[i]);
		}
	}

	for (uint32_t i = 0; i < 3; ++i) {
		delay_o[i] = *(self->p_delay[i+4]);
		if (delay_o[i] != self->dly_o[i].c_dly) {
			/* output delay-time changed */
			fade_o[i] = fade_len;
		}
	}

	/** prepare trigger output **/
	switch (trigger_mode) {
		case TRG_LTC:
		case TRG_EDGE:
		case TRG_PULSEWIDTH:
		case TRG_PULSETRAIN:
		case TRG_WINDOW_ENTER:
		case TRG_WINDOW_LEAVE:
		case TRG_HYSTERESIS:
		case TRG_RUNT:
		case TRG_DROPIN:
		case TRG_DROPOUT:
			/* clear channel */
			memset(a_o[3], 0, sizeof(float) * n_samples);
			break;
		case TRG_PASSTRHU:
		case TRG_RMS:
		case TRG_LPF:
		default:
			break;
	}

	/* process every sample */
	for (uint32_t n = 0; n < n_samples; ++n) {
		float d_i[4], d_o[4], ain[4];

#define DELAYLINE_INC(IN, OUT, IO, CHN) \
		self->dly_ ## IO[CHN].r_ptr = (self->dly_ ## IO[CHN].r_ptr + 1) % MAXDELAY; \
		self->dly_ ## IO[CHN].w_ptr = (self->dly_ ## IO[CHN].w_ptr + 1) % MAXDELAY;

#define DELAYLINE_STEP(IN, OUT, IO, CHN) \
		if (fade_ ## IO[CHN] > 0 && n < 2 * fade_ ## IO[CHN]) { \
			if (n < fade_ ## IO[CHN]) { \
				/* fade out previous signal */ \
				const float gain = (float)(fade_ ## IO[CHN] - n) / (float)fade_ ## IO[CHN]; \
				self->dly_ ## IO[CHN].buffer[ self->dly_ ## IO[CHN].w_ptr ] = IN; \
				OUT = self->dly_ ## IO[CHN].buffer[ self->dly_ ## IO[CHN].r_ptr ] * gain; \
				DELAYLINE_INC(IN, OUT, IO, CHN) \
			} \
			if (n == fade_ ## IO[CHN]) { \
				/* switch read pointer */ \
				self->dly_ ## IO[CHN].r_ptr += self->dly_ ## IO[CHN].c_dly - delay_ ## IO[CHN]; \
				if (self->dly_ ## IO[CHN].r_ptr < 0) { \
					self->dly_ ## IO[CHN].r_ptr -= MAXDELAY * floor(self->dly_ ## IO[CHN].r_ptr / (float)MAXDELAY); \
				} \
				self->dly_ ## IO[CHN].r_ptr = self->dly_ ## IO[CHN].r_ptr % MAXDELAY; \
				self->dly_ ## IO[CHN].c_dly = delay_ ## IO[CHN]; \
			} \
			if (n >= fade_ ## IO[CHN]) { \
				/* fade in at new position */ \
				const float gain = (float)(n - fade_ ## IO[CHN]) / (float)fade_ ## IO[CHN]; \
				self->dly_ ## IO[CHN].buffer[ self->dly_ ## IO[CHN].w_ptr ] = IN; \
				OUT = self->dly_ ## IO[CHN].buffer[ self->dly_ ## IO[CHN].r_ptr ] * gain; \
				DELAYLINE_INC(IN, OUT, IO, CHN) \
			} \
		} else { \
			self->dly_ ## IO[CHN].buffer[ self->dly_ ## IO[CHN].w_ptr ] = IN; \
			OUT = self->dly_ ## IO[CHN].buffer[ self->dly_ ## IO[CHN].r_ptr ]; \
			DELAYLINE_INC(IN, OUT, IO, CHN) \
		}

#define PREFILTER(IN, OUT, CHN) \
		flt_y[CHN] = flt_alpha * (flt_y[CHN] + IN[CHN] - flt_z[CHN]); \
		flt_z[CHN] = IN[CHN]; \
		switch(cmode_in[CHN]) { \
			case 0: OUT[CHN] = IN[CHN]; break; \
			case 2: OUT[CHN] = flt_y[CHN]; break; \
			default: OUT[CHN] = 0; break; \
		} \
		if (cmode_in[CHN] != pmode_in[CHN] && n < fade_len) { \
			const float gain = (float)(fade_len - n) / fade_len; \
			switch(pmode_in[CHN]) { \
				case 0: OUT[CHN] = IN[CHN] * gain; break; \
				case 2: OUT[CHN] = flt_y[CHN] * gain; break; \
				default: OUT[CHN] = 0; break; \
			} \
		} \
		else if (cmode_in[CHN] != pmode_in[CHN] && n < 2 * fade_len) { \
			const float gain = (float)(n - fade_len) / fade_len; \
			OUT[CHN] *= gain; \
		} \
		amp_in[CHN] += mix_alpha * (amp_in_t[CHN] - amp_in[CHN]); \
		OUT[CHN] *= amp_in[CHN];

		/* input delaylines */
		DELAYLINE_STEP(a_i[0][n], ain[0], i, 0)
		DELAYLINE_STEP(a_i[1][n], ain[1], i, 1)
		DELAYLINE_STEP(a_i[2][n], ain[2], i, 2)
		DELAYLINE_STEP(a_i[3][n], ain[3], i, 3)

		/* filter | mute */
		PREFILTER(ain, d_i, 0)
		PREFILTER(ain, d_i, 1)
		PREFILTER(ain, d_i, 2)
		PREFILTER(ain, d_i, 3)

		/* process trigger */
		switch (trigger_mode) {
			case TRG_EDGE:
				if (edge_trigger(tt_edgemode, tt_level0, ts_prev, d_i[trigger_chn])) {
					a_o[3][n] = FIRE_TRIGGER;
				}
				ts_prev = d_i[trigger_chn];
				break;
			case TRG_LTC:
				ltc_decoder_write_float(self->decoder, &d_i[trigger_chn], 1, n + monotonic_cnt);
				break;
			case TRG_PULSEWIDTH:
				/* - prev. edge-trigger is between min,max time ago
				 * % trigger-level, edge-mode, time min,max
				 */
				if (edge_trigger(tt_edgemode, tt_level0, ts_prev, d_i[trigger_chn])) {
					if (monotonic_cnt + n >= ts_time + tt_tme0 && monotonic_cnt + n <= ts_time + tt_tme1) {
						a_o[3][n] = FIRE_TRIGGER;
					}
					ts_time = monotonic_cnt + n;
				}
				ts_prev = d_i[trigger_chn];
				break;
			case TRG_PULSETRAIN:
				/* - no trigger for a given time (max)
				 * - more than one trigger for given time (min)
				 * % trigger-level, edge-mode, time min,max
				 */
				if (edge_trigger(tt_edgemode, tt_level0, ts_prev, d_i[trigger_chn])) {
					if (monotonic_cnt + n < ts_time + tt_tme0) {
						a_o[3][n] = FIRE_TRIGGER;
					}
					ts_time = monotonic_cnt + n;
				}
				if (monotonic_cnt + n > ts_time + tt_tme1 ) {
					a_o[3][n] = FIRE_TRIGGER;
					ts_time = 1e18; // XXX screws up above < tme0 !?
				}
				ts_prev = d_i[trigger_chn];
				break;
			case TRG_WINDOW_ENTER:
				/* - fire is signal enters a given range
				 * % trigger-level A, trigger-level B, enter||leave
				 */
				{
					const int rt = range_trigger(tt_level0, tt_level1, ts_prev, d_i[trigger_chn]);
					if ((rt&4) == 0 && (rt & tt_edgemode) != 0) {
						a_o[3][n] = FIRE_TRIGGER;
					}
				}
				ts_prev = d_i[trigger_chn];
				break;
			case TRG_WINDOW_LEAVE:
				/* - fire is signal leaves a certain range
				 * % trigger-level A, trigger-level B, enter||leave
				 */
				{
					const int rt = range_trigger(tt_level0, tt_level1, ts_prev, d_i[trigger_chn]);
					if ((rt&4) == 4 && (rt & tt_edgemode) != 0) {
						a_o[3][n] = FIRE_TRIGGER;
					}
				}
				ts_prev = d_i[trigger_chn];
				break;
			case TRG_DROPOUT:
				/* - fire is signal leaves a given range for at least a given time
				 * % trigger-level A, trigger-level B, enter||leave, timeout
				 */
				{
					const int rt = range_trigger(tt_level0, tt_level1, ts_prev, d_i[trigger_chn]);
					if ((rt&4) == 4 && (rt & tt_edgemode) != 0) {
						ts_time = monotonic_cnt + n;
					}
				}
				if (monotonic_cnt + n > ts_time + tt_tme0) {
					a_o[3][n] = FIRE_TRIGGER;
					ts_time = 1e18;
				}
				ts_prev = d_i[trigger_chn];
				break;
			case TRG_DROPIN:
				/* - fire is signal enters a given range for at least a given time
				 * % trigger-level A, trigger-level B, enter||leave, timeout
				 */
				{
					const int rt = range_trigger(tt_level0, tt_level1, ts_prev, d_i[trigger_chn]);
					if ((rt&4) == 0 && (rt & tt_edgemode) != 0) {
						// in range -- start counting (if not counting already)
						if (ts_time == 0) {
							ts_time = monotonic_cnt + n;
						}
					} else {
						// out of range -- reset count
						ts_time = 0;
					}
				}
				if (monotonic_cnt + n > ts_time + tt_tme0) {
					a_o[3][n] = FIRE_TRIGGER;
					ts_time = 1e18;
				}
				ts_prev = d_i[trigger_chn];
				break;
			case TRG_HYSTERESIS:
				/* - fire if signal cross both min,max in same direction w/o interruption
				 * % trigger-level A, trigger-level B, edge-mode
				 */
				// TODO: consolidate calls to edge_trigger()
				if (tt_edgemode & 1) { // rising edge
					if ((ts_hysteresis & 1) == 0) {
						// check first edge
						if (edge_trigger(1, tt_level0, ts_prev, d_i[trigger_chn])) {
							ts_hysteresis |= 1;
						}
					}
					else if ((ts_hysteresis & 1) == 1) {
						// check 2nd edge -- or inverse 1st edge -> reset
						if (edge_trigger(1, tt_level1, ts_prev, d_i[trigger_chn])) {
							// fire
							ts_hysteresis &= ~1;
							a_o[3][n] = FIRE_TRIGGER;
						}
						else
						if (edge_trigger(2, tt_level0, ts_prev, d_i[trigger_chn])) {
							ts_hysteresis &= ~1;
						}
					}
				}

				if (tt_edgemode & 2) { // falling edge
					if ((ts_hysteresis & 2) == 0) {
						// check first edge
						if (edge_trigger(2, tt_level1, ts_prev, d_i[trigger_chn])) {
							ts_hysteresis |= 2;
						}
					}
					else if ((ts_hysteresis & 2) == 2) {
						// check 2nd edge -- or inverse 1st edge -> reset
						if (edge_trigger(2, tt_level0, ts_prev, d_i[trigger_chn])) {
							// fire
							ts_hysteresis &= ~2;
							a_o[3][n] = FIRE_TRIGGER;
						}
						else
						if (edge_trigger(1, tt_level1, ts_prev, d_i[trigger_chn])) {
							ts_hysteresis &= ~2;
						}
					}
				}
				ts_prev = d_i[trigger_chn];
				break;
			case TRG_RUNT:
				/* - fire is signal crosses 1st but not 2nd threshold
				 * % trigger-level A, trigger-level B, edge-direction(s)
				 */
				if (tt_edgemode & 1) { // rising edge
					if ((ts_hysteresis & 1) == 0) {
						// check first edge
						if (edge_trigger(1, tt_level0, ts_prev, d_i[trigger_chn])) {
							ts_hysteresis |= 1;
						}
					}
					else if ((ts_hysteresis & 1) == 1) {
						int rt = range_trigger(tt_level0, tt_level1, ts_prev, d_i[trigger_chn]);
						//assert (rt & 4);
						if (rt & 2) {
							ts_hysteresis &= ~1;
							a_o[3][n] = FIRE_TRIGGER;
						} else if (rt & 1) {
							ts_hysteresis &= ~1;
						}
					}
				}
				if (tt_edgemode & 2) { // falling edge
					if ((ts_hysteresis & 2) == 0) {
						// check first edge
						if (edge_trigger(2, tt_level1, ts_prev, d_i[trigger_chn])) {
							ts_hysteresis |= 2;
						}
					}
					else if ((ts_hysteresis & 2) == 2) {
						int rt = range_trigger(tt_level0, tt_level1, ts_prev, d_i[trigger_chn]);
						//assert (rt & 4);
						if (rt & 1) {
							ts_hysteresis &= ~2;
							a_o[3][n] = FIRE_TRIGGER;
						} else if (rt & 2) {
							ts_hysteresis &= ~2;
						}
					}
				}
				ts_prev = d_i[trigger_chn];
				break;
			case TRG_RMS:
				ts_prev += tri_alpha * ((d_i[trigger_chn] * d_i[trigger_chn]) - ts_prev);
				a_o[3][n] = sqrtf(ts_prev);
				break;
			case TRG_LPF:
				ts_prev += tri_alpha * (d_i[trigger_chn] - ts_prev);
				a_o[3][n] = ts_prev;
				break;
			default:
				a_o[3][n] = d_i[trigger_chn];
				break;
		}

		for (uint32_t i = 0; i < 12; ++i) {
			/* low-pass filter gain */
			mix[i] += mix_alpha * (mix_t[i] - mix[i]);
		}

		/* mix matrix */
		d_o[0] = d_i[0] * mix[0] + d_i[1] * mix[3] + d_i[2] * mix[6] + d_i[3] * mix[ 9];
		d_o[1] = d_i[0] * mix[1] + d_i[1] * mix[4] + d_i[2] * mix[7] + d_i[3] * mix[10];
		d_o[2] = d_i[0] * mix[2] + d_i[1] * mix[5] + d_i[2] * mix[8] + d_i[3] * mix[11];

		/* output delaylines */
		DELAYLINE_STEP(d_o[0], a_o[0][n], o, 0)
		DELAYLINE_STEP(d_o[1], a_o[1][n], o, 1)
		DELAYLINE_STEP(d_o[2], a_o[2][n], o, 2)
	}

	/* post-process trigger */
	switch (trigger_mode) {
		case TRG_LTC:
			{
				LTCFrameExt frame;
				while (ltc_decoder_read(self->decoder,&frame)) {
					if (frame.off_end >= monotonic_cnt && frame.off_end < monotonic_cnt + n_samples) {
						const int nf = frame.off_end - monotonic_cnt;
						a_o[3][nf] = FIRE_TRIGGER;
					}
				}
			}
			break;
		case TRG_LPF:
			if (isfinite(ts_prev)) {
				ts_prev += 1e-20;
			} else {
				ts_prev = 0;
			}
			break;
		default:
			break;
	}

	/* copy back filter vars */
	for (uint32_t i = 0; i < 4; ++i) {
		self->flt_z[i] = flt_z[i];
		if (isfinite(flt_y[i])) {
			self->flt_y[i] = flt_y[i];
		} else {
			self->flt_y[i] = 0;
		}

		if (isfinite(amp_in[i])
				&& fabsf(amp_in[i]) > 0.0001 /* -80dB */) {
			self->amp_z[i] = amp_in[i] + 1e-20;
		} else {
			self->amp_z[i] = 0;
		}

		self->mode_input[i] = cmode_in[i];
	}

	for (uint32_t i = 0; i < 12; ++i) {
		if (isfinite(mix[i])
				&& fabsf(mix[i]) > 0.0001 /* -80dB */) {
			self->mix_z[i] = mix[i] + 1e-20;
		} else {
			self->mix_z[i] = 0;
		}
	}
	self->monotonic_cnt += n_samples;
	self->ts_time = ts_time;
	self->ts_prev = ts_prev;
	self->ts_hysteresis = ts_hysteresis;
}

static void
cleanup(LV2_Handle handle)
{
	MixTri* self = (MixTri*)handle;
	ltc_decoder_free(self->decoder);
	free(handle);
}


/******************************************************************************
 * LV2 setup
 */

const void*
extension_data(const char* uri)
{
	return NULL;
}

#define mkdesc_mixtri(ID, NAME) \
static const LV2_Descriptor descriptor ## ID = { \
	MIXTRI_URI NAME,     \
	instantiate,         \
	connect_port_mixtri, \
	NULL,                \
	run,                 \
	NULL,                \
	cleanup,             \
	extension_data       \
};

mkdesc_mixtri(0, "lv2")
mkdesc_mixtri(1, "lv2_gtk")

LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch (index) {
		case  0: return &descriptor0;
		case  1: return &descriptor1;
		default: return NULL;
	}
}

/* vi:set ts=2 sts=2 sw=2: */
