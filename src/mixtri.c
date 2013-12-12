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
	float* a_out[5];

	float* p_mix[16];
	float* p_delay[8];
	float* p_input[4];
	float* p_trigger_chn;
	float* p_trigger_mode;

	double rate;
	DelayLine dly_i[4];
	DelayLine dly_o[4];

	int   mode_input[4];
	float flt_z[4];
	float flt_y[4];
	float flt_alpha;

	LTCDecoder *decoder;
	uint64_t monotonic_cnt;

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

	self->flt_alpha = 1.0 - 5.0 / rate;

	for (uint32_t i = 0; i < 4; ++i) {
		self->mode_input[i] = 0;
		self->flt_z[i] = 0;
		self->flt_y[i] = 0;
		memset(self->dly_i[i].buffer, 0, sizeof(float) * MAXDELAY);
		memset(self->dly_o[i].buffer, 0, sizeof(float) * MAXDELAY);
	}

	self->decoder = ltc_decoder_create(rate / 25, 8);
	self->monotonic_cnt = 0;
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
		default:
			if (port >= MIXTRI_AUDIO_IN_0 && port <= MIXTRI_AUDIO_IN_3) {
				self->a_in[port - MIXTRI_AUDIO_IN_0] = (float*)data;
			}
			else if (port >= MIXTRI_AUDIO_OUT_0 && port <= MIXTRI_AUDIO_OUT_T) {
				self->a_out[port - MIXTRI_AUDIO_OUT_0] = (float*)data;
			}
			else if (port >= MIXTRI_MIX_0_0 && port <= MIXTRI_MIX_3_3) {
				self->p_mix[port - MIXTRI_MIX_0_0] = (float*)data;
			}
			else if (port >= MIXTRI_DLY_I_0 && port <= MIXTRI_DLY_O_3) {
				self->p_delay[port - MIXTRI_DLY_I_0] = (float*)data;
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

	float const * const * a_i = (float const * const *) self->a_in;
	float * const * a_o = self->a_out;
	float mix[16];
	int fade_i[4] = { 0, 0, 0, 0};
	int fade_o[4] = { 0, 0, 0, 0};
	int delay_i[4], delay_o[4];
	float flt_z[4], flt_y[4];
	int cmode_in[4];
	int pmode_in[4];

	const uint64_t monotonic_cnt = self->monotonic_cnt;
	const float fade_len = (n_samples >= FADE_LEN) ? FADE_LEN : ceilf(n_samples / 2.f);
	const float alpha = self->flt_alpha;
	const int trigger_mode = *self->p_trigger_mode;
	const int trigger_chn  = MIN(3, MAX(0, *self->p_trigger_chn));

	for (uint32_t i = 0; i < 16; ++i) {
		mix[i] = *(self->p_mix[i]);
	}
	for (uint32_t i = 0; i < 4; ++i) {
		flt_z[i] = self->flt_z[i];
		flt_y[i] = self->flt_y[i];
		cmode_in[i] = *(self->p_input[i]);
		pmode_in[i] = self->mode_input[i];

		delay_i[i] = *(self->p_delay[i]);
		delay_o[i] = *(self->p_delay[i+4]);

		if (delay_i[i] != self->dly_i[i].c_dly) {
			fade_i[i] = fade_len;
		}
		if (delay_o[i] != self->dly_o[i].c_dly) {
			fade_o[i] = fade_len;
		}
	}

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
		flt_y[CHN] = alpha * (flt_y[CHN] + IN[CHN] - flt_z[CHN]); \
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
		}

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

		switch (trigger_mode) {
			case 1:
				ltc_decoder_write_float(self->decoder, &d_i[trigger_chn], 1, n + monotonic_cnt);
				break;
			default:
				a_o[4][n] = d_i[trigger_chn];
				break;
		}

		/* mix matrix */
		d_o[0] = d_i[0] * mix[0] + d_i[1] * mix[4] + d_i[2] * mix[ 8] + d_i[3] * mix[12];
		d_o[1] = d_i[0] * mix[1] + d_i[1] * mix[5] + d_i[2] * mix[ 9] + d_i[3] * mix[13];
		d_o[2] = d_i[0] * mix[2] + d_i[1] * mix[6] + d_i[2] * mix[10] + d_i[3] * mix[14];
		d_o[3] = d_i[0] * mix[3] + d_i[1] * mix[7] + d_i[2] * mix[11] + d_i[3] * mix[15];

		/* output delaylines */
		DELAYLINE_STEP(d_o[0], a_o[0][n], o, 0)
		DELAYLINE_STEP(d_o[1], a_o[1][n], o, 1)
		DELAYLINE_STEP(d_o[2], a_o[2][n], o, 2)
		DELAYLINE_STEP(d_o[3], a_o[3][n], o, 3)
	}

	/* copy back filter vars */
	for (uint32_t i = 0; i < 4; ++i) {
		self->flt_z[i] = flt_z[i];
		if (isfinite(flt_y[i])) {
			self->flt_y[i] = flt_y[i];
		} else {
			self->flt_y[i] = 0;
		}
		self->mode_input[i] = cmode_in[i];
	}

	switch (trigger_mode) {
		case 1:
			{
				LTCFrameExt frame;
				memset(a_o[4], 0, sizeof(float) * n_samples);
				while (ltc_decoder_read(self->decoder,&frame)) {
					if (frame.off_end >= monotonic_cnt && frame.off_end < monotonic_cnt + n_samples) {
						const int nf = frame.off_end - monotonic_cnt;
						a_o[4][nf] = 1.0;
					}
				}
			}
			break;
		default:
			break;
	}
	self->monotonic_cnt += n_samples;
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
