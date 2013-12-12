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

#define MIX_WIDTH  80
#define MIX_HEIGHT 40
#define MIX_RADIUS 10
#define MIX_CX 39.5
#define MIX_CY 16.5

typedef struct {
	LV2UI_Write_Function write;
	LV2UI_Controller controller;

	RobWidget *hbox, *ctable;

	RobTkLbl  *lbl_in[4];
	RobTkLbl  *lbl_out[3];
	RobTkLbl  *label[8];
	RobTkDial *dial_in[4];
	RobTkDial *dial_mix[12];
	RobTkSpin *spb_delay_in[4];
	RobTkSpin *spb_delay_out[3];
	RobTkCBtn *btn_hpfilt_in[4];
	RobTkCBtn *btn_mute_in[4];
	RobTkRBtn *btn_trig_src[4];
	RobTkSelect *sel_trig_mode;
	bool disable_signals;

	PangoFontDescription *font;

	cairo_surface_t* routeT;
	cairo_surface_t* routeC;
	cairo_surface_t* routeM;
	cairo_surface_t* routeE;
	cairo_surface_t* routeI;
	cairo_surface_t* delayI;
	cairo_surface_t* delayO;

} MixTriUI;

static void create_faceplate(MixTriUI *ui) {
	cairo_t* cr;
	float xlp, ylp;
	PangoFontDescription *font = pango_font_description_from_string("Sans 6");

#define AMPLABEL(V, O, T, X) \
	{ \
		float ang = (-.75 * M_PI) + (1.5 * M_PI) * ((V) + (O)) / (T); \
		xlp = X + .5 + sinf (ang) * (MIX_RADIUS + 3.0); \
		ylp = MIX_CY + .5 - cosf (ang) * (MIX_RADIUS + 3.0); \
		cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND); \
		CairoSetSouerceRGBA(c_wht); \
		cairo_set_line_width(cr, 1.5); \
		cairo_move_to(cr, rint(xlp)-.5, rint(ylp)-.5); \
		cairo_close_path(cr); \
		cairo_stroke(cr); \
		xlp = X + .5 + sinf (ang) * (MIX_RADIUS + 9.5); \
		ylp = MIX_CY + .5 - cosf (ang) * (MIX_RADIUS + 9.5); \
	}

#define COMMONRROUTE(SF, TOP, RIGHT) \
	SF = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, MIX_WIDTH, MIX_HEIGHT); \
	cr = cairo_create (SF); \
	cairo_set_source_rgba (cr, .3, .3, .4, 1.0); \
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
	AMPLABEL(-5.623, 6., 12., MIX_CX); write_text_full(cr, "+15 ", font, xlp, ylp, 0, -2, c_wht); \
	AMPLABEL(-3.162, 6., 12., MIX_CX); write_text_full(cr, "\u00D8", font, xlp, ylp, 0, -2, c_wht); \
	AMPLABEL(3.162, 6., 12., MIX_CX); AMPLABEL(0, 6., 12., MIX_CX); \
	AMPLABEL(-1, 6., 12., MIX_CX); AMPLABEL(1, 6., 12., MIX_CX); \
	AMPLABEL(-1.995, 6., 12., MIX_CX); AMPLABEL(1.995, 6., 12., MIX_CX); \
	AMPLABEL(5.623, 6., 12., MIX_CX); write_text_full(cr, "+15", font, xlp, ylp, 0, -2, c_wht); \
	cairo_destroy (cr);

	COMMONRROUTE(ui->routeM, 0, MIX_WIDTH);
	COMMONRROUTE(ui->routeE, 0, MIX_CX);
	COMMONRROUTE(ui->routeT, MIX_CY, MIX_WIDTH);
	COMMONRROUTE(ui->routeC, MIX_CY, MIX_CX);


	const double dashed[] = {2.5};
#define DASHEDROUTE(SF) \
	cr = cairo_create (SF); \
	CairoSetSouerceRGBA(c_g60); \
	cairo_set_line_width(cr, 1.0); \
	cairo_set_dash(cr, dashed, 1, 0); \
	cairo_move_to(cr, MIX_CX, MIX_CY); \
	cairo_line_to(cr, MIX_WIDTH, MIX_CY); \
	cairo_stroke(cr); \
	cairo_move_to(cr, MIX_WIDTH-15.5, MIX_CY-3.5); \
	cairo_line_to(cr, MIX_WIDTH-15.5, MIX_CY+3.5); \
	cairo_line_to(cr, MIX_WIDTH-9.5, MIX_CY); \
	cairo_close_path(cr); \
	cairo_fill(cr); \
	cairo_destroy (cr);

	DASHEDROUTE(ui->routeE)
	DASHEDROUTE(ui->routeC)

	ui->routeI = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 60, MIX_HEIGHT);
	cr = cairo_create (ui->routeI);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle (cr, 0, 0, 2, MIX_HEIGHT);
	cairo_set_source_rgba (cr, .0, .0, .0, .0);
	cairo_fill (cr);
	cairo_rectangle (cr, 2, 0, MIX_WIDTH-2, MIX_HEIGHT);
	cairo_set_source_rgba (cr, .3, .4, .3, 1.0);
	cairo_fill (cr);
	CairoSetSouerceRGBA(c_g60);
	cairo_set_line_width(cr, 1.0);
	cairo_move_to(cr, 0, MIX_CY);
	cairo_line_to(cr, 60, MIX_CY);
	cairo_stroke(cr);
#ifndef GTK_BACKEND
	cairo_move_to(cr, 6.5, MIX_CY-3.5);
	cairo_line_to(cr, 6.5, MIX_CY+3.5);
	cairo_line_to(cr, 12.5, MIX_CY);
	cairo_close_path(cr);
	cairo_move_to(cr, 60-7.5, MIX_CY-3.5);
	cairo_line_to(cr, 60-7.5, MIX_CY+3.5);
	cairo_line_to(cr, 60-1.5, MIX_CY);
	cairo_close_path(cr);
	cairo_fill(cr);
#endif
	AMPLABEL(  0, 60., 80., 30.5); write_text_full(cr, " 0dB", font, xlp, ylp, 0, -2, c_wht);
	AMPLABEL( 20, 60., 80., 30.5); write_text_full(cr, "+20", font, xlp, ylp, 0, -2, c_wht);
	AMPLABEL(-60, 60., 80., 30.5); write_text_full(cr, "-60", font, xlp, ylp, 0, -2, c_wht);
	AMPLABEL(-50, 60., 80., 30.5);
	AMPLABEL(-40, 60., 80., 30.5);
	AMPLABEL(-30, 60., 80., 30.5);
	AMPLABEL(-20, 60., 80., 30.5);
	AMPLABEL(-10, 60., 80., 30.5);
	AMPLABEL( 10, 60., 80., 30.5);
	AMPLABEL( 20, 60., 80., 30.5);
	pango_font_description_free(font);

	ui->delayO = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, MIX_WIDTH, GSP_HEIGHT);
	cr = cairo_create (ui->delayO);
	cairo_set_source_rgba (cr, .3, .2, .4, 1.0);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle (cr, 0, 0, MIX_WIDTH, GSP_HEIGHT);
	cairo_fill (cr);
	cairo_set_line_width(cr, 1.0);
	CairoSetSouerceRGBA(c_blk);
	cairo_move_to(cr, MIX_CX, 0);
	cairo_line_to(cr, MIX_CX, GSP_HEIGHT);
	cairo_stroke(cr);
	cairo_move_to(cr, MIX_CX-3, GSP_HEIGHT-6.5);
	cairo_line_to(cr, MIX_CX+3, GSP_HEIGHT-6.5);
	cairo_line_to(cr, MIX_CX, GSP_HEIGHT-0.5);
	cairo_close_path(cr);
	cairo_fill(cr);
	cairo_destroy (cr);

	ui->delayI = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, MIX_WIDTH, GSP_HEIGHT);
	cr = cairo_create (ui->delayI);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba (cr, .0, .0, .0, .0);
	cairo_rectangle (cr, 0, 0, MIX_WIDTH, GSP_HEIGHT);
	cairo_fill (cr);
	cairo_set_source_rgba (cr, .4, .3, .3, 1.0);
	cairo_rectangle (cr, 0, 0, MIX_WIDTH-4, GSP_HEIGHT);
	cairo_fill (cr);
	cairo_set_line_width(cr, 1.0);
	CairoSetSouerceRGBA(c_g60);
	cairo_set_line_width(cr, 1.0);
	cairo_move_to(cr, 0, GSP_CY);
	cairo_line_to(cr, MIX_WIDTH-3, GSP_CY);
	cairo_stroke(cr);
	cairo_destroy (cr);

}

static void annotation_txt(MixTriUI *ui, RobTkDial * d, cairo_t *cr, const char *txt) {
	int tw, th;
	cairo_save(cr);
	PangoLayout * pl = pango_cairo_create_layout(cr);
	pango_layout_set_font_description(pl, ui->font);
	pango_layout_set_text(pl, txt, -1);
	pango_layout_get_pixel_size(pl, &tw, &th);
	cairo_translate (cr, d->w_cx, d->w_height);
	cairo_translate (cr, -tw/2.0 - 0.5, -th);
	cairo_set_source_rgba (cr, .0, .0, .0, .7);
	rounded_rectangle(cr, -1, -1, tw+3, th+1, 3);
	cairo_fill(cr);
	CairoSetSouerceRGBA(c_wht);
	pango_cairo_layout_path(cr, pl);
	pango_cairo_show_layout(cr, pl);
	g_object_unref(pl);
	cairo_restore(cr);
	cairo_new_path(cr);
}

static void dial_annotation_val(RobTkDial * d, cairo_t *cr, void *data) {
	MixTriUI* ui = (MixTriUI*) (data);
	char tmp[16];

	if (d->cur == 0) {
		snprintf(tmp, 16, "-\u221EdB");
	} else if (d->cur > 0) {
		snprintf(tmp, 16, "%+4.1fdB", 20 * log10f(d->cur));
	} else {
		snprintf(tmp, 16, "\u00D8%+4.1fdB", 20 * log10f(-d->cur));
	}
	annotation_txt(ui, d, cr, tmp);
}

static void dial_annotation_db(RobTkDial * d, cairo_t *cr, void *data) {
	MixTriUI* ui = (MixTriUI*) (data);
	char tmp[16];
	snprintf(tmp, 16, "%+4.1fdB", d->cur);
	annotation_txt(ui, d, cr, tmp);
}

#ifndef GTK_BACKEND
static bool box_expose_event(RobWidget* rw, cairo_t* cr, cairo_rectangle_t *ev) {
	if (rw->resized) {
		cairo_rectangle_t event;
		event.x = MAX(0, ev->x - rw->area.x);
		event.y = MAX(0, ev->y - rw->area.y);
		event.width  = MIN(rw->area.x + rw->area.width , ev->x + ev->width)   - MAX(ev->x, rw->area.x);
		event.height = MIN(rw->area.y + rw->area.height, ev->y + ev->height) - MAX(ev->y, rw->area.y);
		cairo_save(cr);
		rcontainer_clear_bg(rw, cr, &event);

		cairo_set_source_rgba (cr, .4, .3, .3, 1.0);
		cairo_rectangle (cr, 32, 17, 78, 160);
		cairo_fill(cr);

		cairo_set_source_rgba (cr, .4, .3, .3, 1.0);
		cairo_rectangle (cr, 108, 17, 110, 160);
		cairo_fill(cr);

		cairo_set_source_rgba (cr, .2, .3, .35, 1.0);
		cairo_rectangle (cr, 516, 17, 60, 160+30);
		cairo_fill(cr);

		const double dashed[] = {2.5};
		cairo_set_line_width(cr, 1.0);
		cairo_set_dash(cr, dashed, 1, 4);
		CairoSetSouerceRGBA(c_g60);
		for (uint32_t i = 0; i < 4; ++i) {
			cairo_move_to(cr, 514,      33.5 + i*40);
			cairo_line_to(cr, 514 + 32, 33.5 + i*40);
			cairo_stroke(cr);
		}

		cairo_restore(cr);
	}
	return rcontainer_expose_event_no_clear(rw, cr, ev);
}
#endif

/******************************************************************************
 * UI callbacks
 */

static bool cb_set_trig_mode (RobWidget* handle, void *data) {
	MixTriUI* ui = (MixTriUI*) (data);
	if (ui->disable_signals) return TRUE;
	float mode = robtk_select_get_value(ui->sel_trig_mode);
	ui->write(ui->controller, MIXTRI_TRIG_MODE, sizeof(float), 0, (const void*) &mode);
	return TRUE;
}

static bool cb_set_trig_chn (RobWidget* handle, void *data) {
	MixTriUI* ui = (MixTriUI*) (data);
	float chn = 0;
	if (ui->disable_signals) return TRUE;
	for (uint32_t i = 0; i < 4; ++i) {
		if (robtk_rbtn_get_active(ui->btn_trig_src[i])) {
			chn = i;
			break;
		}
	}
	ui->write(ui->controller, MIXTRI_TRIG_CHN, sizeof(float), 0, (const void*) &chn);
	return TRUE;
}

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

static bool cb_set_in (RobWidget* handle, void *data) {
	MixTriUI* ui = (MixTriUI*) (data);
	if (ui->disable_signals) return TRUE;
	for (uint32_t i = 0; i < 4; ++i) {
		float val = robtk_dial_get_value(ui->dial_in[i]);
		ui->write(ui->controller, MIXTRI_GAIN_I_0 + i, sizeof(float), 0, (const void*) &val);
	}
	return TRUE;
}

static bool cb_set_mix (RobWidget* handle, void *data) {
	MixTriUI* ui = (MixTriUI*) (data);
	if (ui->disable_signals) return TRUE;
	for (uint32_t i = 0; i < 12; ++i) {
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
	for (uint32_t i = 0; i < 3; ++i) {
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

	ui->font = pango_font_description_from_string("Mono 8");
	create_faceplate(ui);

	ui->label[0] = robtk_lbl_new("Delay [spl]");
	ui->label[1] = robtk_lbl_new("Output Delay [spl] \u2192 ");
	ui->label[2] = robtk_lbl_new("Mixer Matrix [amp]");
	ui->label[3] = robtk_lbl_new("Chnannel mod.");
	ui->label[4] = robtk_lbl_new("Trigger");
	ui->label[5] = robtk_lbl_new("Gain");
	ui->label[6] = robtk_lbl_new("Trig.");
	ui->label[7] = robtk_lbl_new("x42 MixTri LV2 " MIXTRIVERSION);

	robtk_lbl_set_alignment(ui->label[0], 0.5, 0.5);
	robtk_lbl_set_alignment(ui->label[1], 1.0, 0.25);
	robtk_lbl_set_alignment(ui->label[2], 0.5, 0.5);
	robtk_lbl_set_alignment(ui->label[3], 0.5, 0.5);
	robtk_lbl_set_alignment(ui->label[4], 0.5, 0.5);
	robtk_lbl_set_alignment(ui->label[5], 0.5, 0.5);
	robtk_lbl_set_alignment(ui->label[6], 0.5, 0.5);
	robtk_lbl_set_alignment(ui->label[7], 0.0, 0.5);
	robtk_lbl_set_color(ui->label[7], .6, .6, .6, 1.0);

	ui->ctable = rob_table_new(/*rows*/7, /*cols*/ 9, FALSE);
#ifndef GTK_BACKEND
	ui->ctable->expose_event = box_expose_event;
#endif

	rob_table_attach(ui->ctable, robtk_lbl_widget(ui->label[0]),
			1, 2, 0, 1, 0, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach(ui->ctable, robtk_lbl_widget(ui->label[1]),
			2, 5, 5, 6, 0, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach(ui->ctable, robtk_lbl_widget(ui->label[2]),
			5, 8, 0, 1, 0, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach(ui->ctable, robtk_lbl_widget(ui->label[3]),
			2, 4, 0, 1, 0, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach(ui->ctable, robtk_lbl_widget(ui->label[4]),
			8, 9, 6, 7, 0, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach(ui->ctable, robtk_lbl_widget(ui->label[5]),
			4, 5, 0, 1, 0, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach(ui->ctable, robtk_lbl_widget(ui->label[6]),
			8, 9, 0, 1, 0, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach(ui->ctable, robtk_lbl_widget(ui->label[7]),
			0, 3, 6, 7, 0, 0, RTK_EXANDF, RTK_SHRINK);


	for (uint32_t i = 0; i < 4; ++i) {
		ui->dial_in[i] = robtk_dial_new_with_size(-60.0, 20.0, .01,
				60, MIX_HEIGHT, 30.5, MIX_CY, MIX_RADIUS);
		robtk_dial_set_value(ui->dial_in[i], 0);
		robtk_dial_set_surface(ui->dial_in[i],ui->routeI);
		robtk_dial_set_default(ui->dial_in[i], 0);
		robtk_dial_set_callback(ui->dial_in[i], cb_set_in, ui);
		robtk_dial_annotation_callback(ui->dial_in[i], dial_annotation_db, ui);
		rob_table_attach(ui->ctable, robtk_dial_widget(ui->dial_in[i]),
				4, 5, i+1, i+2, 0, 0, RTK_EXANDF, RTK_SHRINK);
	}

	for (uint32_t i = 0; i < 12; ++i) {
		ui->dial_mix[i] = robtk_dial_new_with_size(-6.0, 6.0, .01,
				MIX_WIDTH, MIX_HEIGHT, MIX_CX, MIX_CY, MIX_RADIUS);
		const float g = ((i%3) == (i/3)) ? 1.0 : 0.0;
		robtk_dial_set_default(ui->dial_mix[i], g);
		robtk_dial_set_value(ui->dial_mix[i], g);
		robtk_dial_set_callback(ui->dial_mix[i], cb_set_mix, ui);
		robtk_dial_annotation_callback(ui->dial_mix[i], dial_annotation_val, ui);
		rob_table_attach(ui->ctable, robtk_dial_widget(ui->dial_mix[i]),
				(i%3)+5, (i%3)+6, (i/3)+1, (i/3)+2,
				0, 0, RTK_EXANDF, RTK_SHRINK);
	}

	for (uint32_t i = 0; i < 3; ++i) {
		robtk_dial_set_surface(ui->dial_mix[i], (i%3)==2 ? ui->routeC : ui->routeT);
	}
	for (uint32_t i = 3; i < 12; ++i) {
		robtk_dial_set_surface(ui->dial_mix[i], (i%3)==2 ? ui->routeE : ui->routeM);
	}

	for (uint32_t i = 0; i < 4; ++i) {
		char tmp[16];
		snprintf(tmp, 16, "In %d ", i+1);
		ui->lbl_in[i] = robtk_lbl_new(tmp);
		robtk_lbl_set_alignment(ui->lbl_in[i], 0.0, 0.3);
		robtk_lbl_set_min_geometry(ui->lbl_in[i], 32, 0);

		ui->btn_mute_in[i]  = robtk_cbtn_new("Mute", GBT_LED_LEFT, false);
		ui->btn_hpfilt_in[i]  = robtk_cbtn_new("HPF", GBT_LED_LEFT, false);
		robtk_cbtn_set_alignment(ui->btn_mute_in[i], .5, 0.25);
		robtk_cbtn_set_alignment(ui->btn_hpfilt_in[i], .5, 0.25);
		robtk_cbtn_set_color_on(ui->btn_hpfilt_in[i],  .1, .2, .9);
		robtk_cbtn_set_color_off(ui->btn_hpfilt_in[i], .1, .1, .3);
		robtk_cbtn_set_callback(ui->btn_mute_in[i], cb_set_fm, ui);
		robtk_cbtn_set_callback(ui->btn_hpfilt_in[i], cb_set_fm, ui);
		rob_table_attach(ui->ctable, robtk_lbl_widget(ui->lbl_in[i]),
				0, 1, i+1, i+2, 0, 0, RTK_EXANDF, RTK_EXANDF);
		rob_table_attach(ui->ctable, robtk_cbtn_widget(ui->btn_mute_in[i]),
				2, 3, i+1, i+2, 0, 0, RTK_SHRINK, RTK_SHRINK);
		rob_table_attach(ui->ctable, robtk_cbtn_widget(ui->btn_hpfilt_in[i]),
				3, 4, i+1, i+2, 0, 0, RTK_SHRINK, RTK_SHRINK);

		ui->spb_delay_in[i]  = robtk_spin_new(0, MAXDELAY-1, 1);
		robtk_spin_set_default(ui->spb_delay_in[i], 0);
		robtk_spin_set_value(ui->spb_delay_in[i], 0);
		robtk_spin_set_callback(ui->spb_delay_in[i], cb_set_delay, ui);
		robtk_spin_label_width(ui->spb_delay_in[i], -1, MIX_WIDTH - GSP_WIDTH - 8);
#ifndef GTK_BACKEND
		robtk_dial_set_surface(ui->spb_delay_in[i]->dial,ui->delayI);
		robtk_lbl_set_alignment(ui->spb_delay_in[i]->lbl_r, 0, 0.3);
		robtk_dial_set_alignment(ui->spb_delay_in[i]->dial, .5, 1.0);
		ui->spb_delay_in[i]->rw->yalign = .45;
#endif
		rob_table_attach(ui->ctable, robtk_spin_widget(ui->spb_delay_in[i]),
				1, 2, i+1, i+2, 0, 0, RTK_EXANDF, RTK_SHRINK);
	}

	for (uint32_t i = 0; i < 3; ++i) {
		char tmp[16];
		snprintf(tmp, 16, "Out %d", i+1);
		ui->lbl_out[i] = robtk_lbl_new(tmp);
		robtk_lbl_set_alignment(ui->lbl_out[i], 0.5, 0.5);
		rob_table_attach(ui->ctable, robtk_lbl_widget(ui->lbl_out[i]),
				5+i, 6+i, 6, 7, 0, 0, RTK_EXANDF, RTK_SHRINK);

		ui->spb_delay_out[i] = robtk_spin_new(0, MAXDELAY-1, 1);
		robtk_spin_set_callback(ui->spb_delay_out[i], cb_set_delay, ui);
		robtk_spin_set_default(ui->spb_delay_out[i], 0);
		robtk_spin_set_value(ui->spb_delay_out[i], 0);
#ifndef GTK_BACKEND
		robtk_dial_set_surface(ui->spb_delay_out[i]->dial,ui->delayO);
		robtk_lbl_set_alignment(ui->spb_delay_out[i]->lbl_r, 0, 0.3);
#endif
		robtk_spin_label_width(ui->spb_delay_out[i], -1, MIX_WIDTH - GSP_WIDTH - 8);
		rob_table_attach(ui->ctable, robtk_spin_widget(ui->spb_delay_out[i]),
				i+5, i+6, 5, 6, 0, 0, RTK_EXANDF, RTK_SHRINK);
	}

	ui->sel_trig_mode = robtk_select_new();
	robtk_select_add_item(ui->sel_trig_mode, 0, "-");
	robtk_select_add_item(ui->sel_trig_mode, 1, "LTC");
	robtk_select_set_callback(ui->sel_trig_mode, cb_set_trig_mode, ui);

	rob_table_attach(ui->ctable, robtk_select_widget(ui->sel_trig_mode),
			8, 9, 5, 6, 0, 0, RTK_SHRINK, RTK_SHRINK);

	ui->btn_trig_src[0] = robtk_rbtn_new("", NULL);
	ui->btn_trig_src[1] = robtk_rbtn_new("", robtk_rbtn_group(ui->btn_trig_src[0]));
	ui->btn_trig_src[2] = robtk_rbtn_new("", robtk_rbtn_group(ui->btn_trig_src[0]));
	ui->btn_trig_src[3] = robtk_rbtn_new("", robtk_rbtn_group(ui->btn_trig_src[0]));

	robtk_rbtn_set_active(ui->btn_trig_src[3], true);

	for (uint32_t i = 0; i < 4; ++i) {
		ui->btn_trig_src[i]->cbtn->flat_button = FALSE;
		robtk_rbtn_set_callback(ui->btn_trig_src[i], cb_set_trig_chn, ui);
		robtk_rbtn_set_alignment(ui->btn_trig_src[i], .5, 0.25);
		rob_table_attach(ui->ctable, robtk_rbtn_widget(ui->btn_trig_src[i]),
				8, 9, i+1, i+2, 0, 0, RTK_EXPAND, RTK_SHRINK);
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

	for (uint32_t i = 0; i < 12; ++i) {
		robtk_dial_destroy(ui->dial_mix[i]);
	}
	for (uint32_t i = 0; i < 4; ++i) {
		robtk_spin_destroy(ui->spb_delay_in[i]);
		robtk_cbtn_destroy(ui->btn_hpfilt_in[i]);
		robtk_cbtn_destroy(ui->btn_mute_in[i]);
		robtk_lbl_destroy(ui->lbl_in[i]);
		robtk_rbtn_destroy(ui->btn_trig_src[i]);
	}
	for (uint32_t i = 0; i < 3; ++i) {
		robtk_spin_destroy(ui->spb_delay_out[i]);
		robtk_lbl_destroy(ui->lbl_out[i]);
	}
	for (uint32_t i = 0; i < 8; ++i) {
		robtk_lbl_destroy(ui->label[i]);
	}
	robtk_select_destroy(ui->sel_trig_mode);
	cairo_surface_destroy(ui->routeT);
	cairo_surface_destroy(ui->routeC);
	cairo_surface_destroy(ui->routeE);
	cairo_surface_destroy(ui->routeM);
	cairo_surface_destroy(ui->routeI);
	cairo_surface_destroy(ui->delayI);
	cairo_surface_destroy(ui->delayO);
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
	if (port >= MIXTRI_MIX_0_0 && port <= MIXTRI_MIX_3_2) {
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
	else if (port >= MIXTRI_DLY_O_0 && port <= MIXTRI_DLY_O_2) {
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
