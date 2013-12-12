/* Mixer+Trigger
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

#define MTR_URI MIXTRI_URI
#define MTR_GUI "ui"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"
#include "src/mixtri.h"

#define MIX_WIDTH  70
#define MIX_HEIGHT 40
#define MIX_RADIUS 10
#define MIX_CX 34.5
#define MIX_CY 16.5

typedef struct {
	LV2UI_Write_Function write;
	LV2UI_Controller controller;

	RobWidget *hbox, *ctable;

	RobTkDial *dial_mix[16];
	RobTkSpin *spb_delay_in[4];
	RobTkSpin *spb_delay_out[5];
	RobTkCBtn *btn_hpfilt_in[4];
	RobTkCBtn *btn_mute_in[4];
	bool disable_signals;

	PangoFontDescription *font;

	cairo_surface_t* routeT;
	cairo_surface_t* routeC;
	cairo_surface_t* routeM;
	cairo_surface_t* routeE;

} MixTriUI;

static void create_faceplate(MixTriUI *ui) {
	cairo_t* cr;
#define COMMONRROUTE(SF, TOP, RIGHT) \
	SF = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, MIX_WIDTH, MIX_HEIGHT); \
	cr = cairo_create (SF); \
	cairo_set_source_rgba (cr, .4, .4, .8, 1.0); \
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE); \
	cairo_rectangle (cr, 0, 0, MIX_WIDTH, MIX_HEIGHT); \
	cairo_fill (cr); \
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER); \
	CairoSetSouerceRGBA(c_g60); \
	cairo_set_line_width(cr, 1.0); \
	cairo_move_to(cr, 0, MIX_CY); \
	cairo_line_to(cr, RIGHT, MIX_CY); \
	cairo_stroke(cr); \
	CairoSetSouerceRGBA(c_blk); \
	cairo_move_to(cr, MIX_CX, TOP); \
	cairo_line_to(cr, MIX_CX, MIX_HEIGHT); \
	cairo_stroke(cr); \
	cairo_move_to(cr, MIX_CX-3, MIX_HEIGHT-6.5); \
	cairo_line_to(cr, MIX_CX+3, MIX_HEIGHT-6.5); \
	cairo_line_to(cr, MIX_CX, MIX_HEIGHT-0.5); \
	cairo_close_path(cr); \
	cairo_fill(cr); \
	cairo_destroy (cr);


	COMMONRROUTE(ui->routeM, 0, MIX_WIDTH);
	COMMONRROUTE(ui->routeE, 0, MIX_CX);
	COMMONRROUTE(ui->routeT, MIX_CY, MIX_WIDTH);
	COMMONRROUTE(ui->routeC, MIX_CY, MIX_CX);
}

static void dial_annotation(RobTkDial * d, cairo_t *cr, void *data) {
	MixTriUI* ui = (MixTriUI*) (data);
	char tmp[16];
	if (d->cur == 0) {
		snprintf(tmp, 16, "-\u221EdB");
	} else if (d->cur > 0) {
		snprintf(tmp, 16, "%+4.1fdB", 20 * log10f(d->cur));
	} else {
		snprintf(tmp, 16, "\u00D8%+4.1fdB", 20 * log10f(-d->cur));
	}
	write_text_full(cr, tmp, ui->font, d->w_cx, d->w_cy, 0, 2, c_wht);
}

/******************************************************************************
 * UI callbacks
 */

static bool cb_set_fm (RobWidget* handle, void *data) {
	MixTriUI* ui = (MixTriUI*) (data);
	if (ui->disable_signals) return TRUE;
	for (uint32_t i = 0; i < 4; ++i) {
		int v = 0; float val;
		if (robtk_cbtn_get_active(ui->btn_mute_in[i])) v|=1;
		if (robtk_cbtn_get_active(ui->btn_hpfilt_in[i])) v|=2;
		val = v;
		ui->write(ui->controller, MIXTRI_MOD_I_0 + i, sizeof(float), 0, (const void*) &val);
	}
	return TRUE;
}

static bool cb_set_mix (RobWidget* handle, void *data) {
	MixTriUI* ui = (MixTriUI*) (data);
	if (ui->disable_signals) return TRUE;
	for (uint32_t i = 0; i < 16; ++i) {
		float val = robtk_dial_get_value(ui->dial_mix[i]);
		ui->write(ui->controller, MIXTRI_MIX_0_0 + i, sizeof(float), 0, (const void*) &val);
	}
	return TRUE;
}

static bool cb_set_delay (RobWidget* handle, void *data) {
	MixTriUI* ui = (MixTriUI*) (data);
	if (ui->disable_signals) return TRUE;
	for (uint32_t i = 0; i < 4; ++i) {
		float val = robtk_spin_get_value(ui->spb_delay_in[i]);
		ui->write(ui->controller, MIXTRI_DLY_I_0 + i, sizeof(float), 0, (const void*) &val);
	}
	for (uint32_t i = 0; i < 4; ++i) {
		float val = robtk_spin_get_value(ui->spb_delay_out[i]);
		ui->write(ui->controller, MIXTRI_DLY_O_0 + i, sizeof(float), 0, (const void*) &val);
	}
	return TRUE;
}

/******************************************************************************
 * RobWidget
 */


static void ui_disable(LV2UI_Handle handle) { }
static void ui_enable(LV2UI_Handle handle) { }

static RobWidget * toplevel(MixTriUI* ui, void * const top)
{
	ui->hbox = rob_hbox_new(FALSE, 2);
	robwidget_make_toplevel(ui->hbox, top);
	ROBWIDGET_SETNAME(ui->hbox, "mixtri");

	ui->font = pango_font_description_from_string("Mono 7");
	create_faceplate(ui);

	ui->ctable = rob_table_new(/*rows*/5, /*cols*/ 7, FALSE);

	for (uint32_t i = 0; i < 16; ++i) {
		ui->dial_mix[i] = robtk_dial_new_with_size(-6.0, 6.0, .01,
				MIX_WIDTH, MIX_HEIGHT, MIX_CX, MIX_CY, MIX_RADIUS);
		const float g = ((i%4) == (i/4)) ? 1.0 : 0.0;
		robtk_dial_set_default(ui->dial_mix[i], g);
		robtk_dial_set_value(ui->dial_mix[i], g);
		robtk_dial_set_callback(ui->dial_mix[i], cb_set_mix, ui);
		robtk_dial_annotation_callback(ui->dial_mix[i], dial_annotation, ui);
		rob_table_attach(ui->ctable, robtk_dial_widget(ui->dial_mix[i]),
				(i%4)+3, (i%4)+4, (i/4), (i/4)+1,
				0, 0, RTK_EXANDF, RTK_SHRINK);
	}

	for (uint32_t i = 0; i < 4; ++i) {
		robtk_dial_set_surface(ui->dial_mix[i], (i%4)==3 ? ui->routeC : ui->routeT);
	}
	for (uint32_t i = 4; i < 16; ++i) {
		robtk_dial_set_surface(ui->dial_mix[i], (i%4)==3 ? ui->routeE : ui->routeM);
	}

	for (uint32_t i = 0; i < 4; ++i) {
		ui->btn_mute_in[i]  = robtk_cbtn_new("Mute", GBT_LED_LEFT, false);
		ui->btn_hpfilt_in[i]  = robtk_cbtn_new("HPF", GBT_LED_LEFT, false);
		robtk_cbtn_set_callback(ui->btn_mute_in[i], cb_set_fm, ui);
		robtk_cbtn_set_callback(ui->btn_hpfilt_in[i], cb_set_fm, ui);
		rob_table_attach(ui->ctable, robtk_cbtn_widget(ui->btn_mute_in[i]),
				1, 2, i, i+1,
				0, 0, RTK_SHRINK, RTK_SHRINK);
		rob_table_attach(ui->ctable, robtk_cbtn_widget(ui->btn_hpfilt_in[i]),
				2, 3, i, i+1,
				0, 0, RTK_SHRINK, RTK_SHRINK);
	}

	for (uint32_t i = 0; i < 4; ++i) {
		ui->spb_delay_in[i]  = robtk_spin_new(0, 1000, 1);
		robtk_spin_set_default(ui->spb_delay_in[i], 0);
		robtk_spin_set_value(ui->spb_delay_in[i], 0);
		robtk_spin_set_callback(ui->spb_delay_in[i], cb_set_delay, ui);
		robtk_spin_label_width(ui->spb_delay_in[i], -1, MIX_WIDTH - GSP_WIDTH - 6);
		rob_table_attach(ui->ctable, robtk_spin_widget(ui->spb_delay_in[i]),
				0, 1, i, i+1,
				0, 0, RTK_EXANDF, RTK_SHRINK);
	}

	for (uint32_t i = 0; i < 4; ++i) {
		ui->spb_delay_out[i] = robtk_spin_new(0, 1000, 1);
		robtk_spin_set_callback(ui->spb_delay_out[i], cb_set_delay, ui);
		robtk_spin_set_default(ui->spb_delay_out[i], 0);
		robtk_spin_set_value(ui->spb_delay_out[i], 0);
		robtk_spin_label_width(ui->spb_delay_out[i], -1, MIX_WIDTH - GSP_WIDTH - 8);
		rob_table_attach(ui->ctable, robtk_spin_widget(ui->spb_delay_out[i]),
				i+3, i+4, 4, 5,
				0, 0, RTK_EXANDF, RTK_SHRINK);
	}

	rob_hbox_child_pack(ui->hbox, ui->ctable, FALSE, FALSE);
	return ui->hbox;
}

/******************************************************************************
 * LV2
 */

static LV2UI_Handle
instantiate(
		void* const               ui_toplevel,
		const LV2UI_Descriptor*   descriptor,
		const char*               plugin_uri,
		const char*               bundle_path,
		LV2UI_Write_Function      write_function,
		LV2UI_Controller          controller,
		RobWidget**               widget,
		const LV2_Feature* const* features)
{
	MixTriUI* ui = (MixTriUI*)calloc(1, sizeof(MixTriUI));

	if (!ui) {
		fprintf(stderr, "MixTri.lv2 UI: out of memory\n");
		return NULL;
	}

	*widget = NULL;

	/* initialize private data structure */
	ui->write      = write_function;
	ui->controller = controller;

	*widget = toplevel(ui, ui_toplevel);

	return ui;
}

static enum LVGLResize
plugin_scale_mode(LV2UI_Handle handle)
{
	return LVGL_LAYOUT_TO_FIT;
}

static void
cleanup(LV2UI_Handle handle)
{
	MixTriUI* ui = (MixTriUI*)handle;
	ui_disable(ui);

	for (uint32_t i = 0; i < 16; ++i) {
		robtk_dial_destroy(ui->dial_mix[i]);
	}
	for (uint32_t i = 0; i < 4; ++i) {
		robtk_spin_destroy(ui->spb_delay_in[i]);
		robtk_spin_destroy(ui->spb_delay_out[i]);
		robtk_cbtn_destroy(ui->btn_hpfilt_in[i]);
		robtk_cbtn_destroy(ui->btn_mute_in[i]);
	}
	cairo_surface_destroy(ui->routeT);
	cairo_surface_destroy(ui->routeC);
	cairo_surface_destroy(ui->routeE);
	cairo_surface_destroy(ui->routeM);
	pango_font_description_free(ui->font);

	rob_box_destroy(ui->ctable);
	rob_box_destroy(ui->hbox);

	free(ui);
}

static void
port_event(LV2UI_Handle handle,
		uint32_t     port,
		uint32_t     buffer_size,
		uint32_t     format,
		const void*  buffer)
{
	MixTriUI* ui = (MixTriUI*)handle;

	if (format != 0) return;
	const float v = *(float *)buffer;
	if (port >= MIXTRI_MIX_0_0 && port <= MIXTRI_MIX_3_3) {
		const int d = port - MIXTRI_MIX_0_0;
		ui->disable_signals = true;
		robtk_dial_set_value(ui->dial_mix[d], v);
		ui->disable_signals = false;
	}
	else if (port >= MIXTRI_DLY_I_0 && port <= MIXTRI_DLY_I_3) {
		const int d = port - MIXTRI_DLY_I_0;
		ui->disable_signals = true;
		robtk_spin_set_value(ui->spb_delay_in[d], v);
		ui->disable_signals = false;
	}
	else if (port >= MIXTRI_DLY_O_0 && port <= MIXTRI_DLY_O_3) {
		const int d = port - MIXTRI_DLY_O_0;
		ui->disable_signals = true;
		robtk_spin_set_value(ui->spb_delay_out[d], v);
		ui->disable_signals = false;
	}
}

	static const void*
extension_data(const char* uri)
{
	return NULL;
}

/* vi:set ts=2 sts=2 sw=2: */
