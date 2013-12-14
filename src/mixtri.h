/* Mixer + Trigger
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIXTRI_H
#define MIXTRI_H

#define MIXTRI_URI "http://gareus.org/oss/lv2/mixtri#"
#define MAXDELAY (192001)

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"

typedef enum {
	MIXTRI_AUDIO_IN_0 = 0,
	MIXTRI_AUDIO_IN_1,
	MIXTRI_AUDIO_IN_2,
	MIXTRI_AUDIO_IN_3,
	MIXTRI_AUDIO_OUT_0,
	MIXTRI_AUDIO_OUT_1,
	MIXTRI_AUDIO_OUT_2,
	MIXTRI_AUDIO_OUT_T,
	MIXTRI_GAIN_I_0,
	MIXTRI_GAIN_I_1,
	MIXTRI_GAIN_I_2,
	MIXTRI_GAIN_I_3,
	MIXTRI_MIX_0_0,
	MIXTRI_MIX_0_1,
	MIXTRI_MIX_0_2,
	MIXTRI_MIX_1_0,
	MIXTRI_MIX_1_1,
	MIXTRI_MIX_1_2,
	MIXTRI_MIX_2_0,
	MIXTRI_MIX_2_1,
	MIXTRI_MIX_2_2,
	MIXTRI_MIX_3_0,
	MIXTRI_MIX_3_1,
	MIXTRI_MIX_3_2,
	MIXTRI_DLY_I_0,
	MIXTRI_DLY_I_1,
	MIXTRI_DLY_I_2,
	MIXTRI_DLY_I_3,
	MIXTRI_DLY_O_0,
	MIXTRI_DLY_O_1,
	MIXTRI_DLY_O_2,
	MIXTRI_MOD_I_0,
	MIXTRI_MOD_I_1,
	MIXTRI_MOD_I_2,
	MIXTRI_MOD_I_3,
	MIXTRI_TRIG_CHN,
	MIXTRI_TRIG_MODE,
	MIXTRI_TRIG_EDGE,
	MIXTRI_TRIG_LVL0,
	MIXTRI_TRIG_LVL1,
	MIXTRI_TRIG_TME0,
	MIXTRI_TRIG_TME1,
} PortIndexMixTri;

enum {
	TRG_PASSTRHU = 0,
	TRG_LTC,
	TRG_EDGE,
	TRG_PULSEWIDTH,
	TRG_PULSETRAIN,
	TRG_WINDOW_ENTER,
	TRG_WINDOW_LEAVE,
	TRG_HYSTERESIS,
	TRG_RUNT,
	TRG_DROPOUT,
	TRG_DROPIN,
	TRG_RMS,
	TRG_LPF,
};

#endif
