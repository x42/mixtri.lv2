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
	RobTkLbl  *label[9];
	RobTkDial *dial_in[4];
	RobTkDial *dial_mix[12];
	RobTkSpin *spb_delay_in[4];
	RobTkSpin *spb_delay_out[3];
	RobTkCBtn *btn_hpfilt_in[4];
	RobTkCBtn *btn_mute_in[4];
	RobTkRBtn *btn_trig_src[4];
	RobTkSelect *sel_trig_mode;

	RobTkLbl  *lbl_trig[4];
	RobTkSelect *sel_trig_edge;
	RobTkSpin *spb_trigger_tme[2];
	RobTkSpin *spb_trigger_lvl[2];
	RobTkDarea *drawing_area;
	RobTkCBtn *btn_show_doc;

	bool disable_signals;

	PangoFontDescription *font[2];

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
	PangoFontDescription *font = pango_font_description_from_string("Sans 8px");

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
	AMPLABEL(-20, 20., 40., MIX_CX); write_text_full(cr, "-20", font, xlp, ylp, 0, 2, c_wht); \
	AMPLABEL( 20, 20., 40., MIX_CX); write_text_full(cr, "+20", font, xlp, ylp, 0, 2, c_wht); \
	AMPLABEL(  0, 20., 40., MIX_CX); \
	AMPLABEL(-12, 20., 40., MIX_CX); \
	AMPLABEL( -6, 20., 40., MIX_CX); \
	AMPLABEL(  0, 20., 40., MIX_CX); \
	AMPLABEL(  6, 20., 40., MIX_CX); \
	AMPLABEL( 12, 20., 40., MIX_CX); \
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

	cairo_move_to(cr, 6.5, MIX_CY-3.5);
	cairo_line_to(cr, 6.5, MIX_CY+3.5);
	cairo_line_to(cr, 12.5, MIX_CY);
	cairo_close_path(cr);
	cairo_move_to(cr, 60-7.5, MIX_CY-3.5);
	cairo_line_to(cr, 60-7.5, MIX_CY+3.5);
	cairo_line_to(cr, 60-1.5, MIX_CY);
	cairo_close_path(cr);
	cairo_fill(cr);

	AMPLABEL(  0, 60., 80., 30.5); write_text_full(cr, " 0dB", font, xlp, ylp, 0, 2, c_wht);
	AMPLABEL( 20, 60., 80., 30.5); write_text_full(cr, "+20", font, xlp, ylp, 0, 2, c_wht);
	AMPLABEL(-60, 60., 80., 30.5); write_text_full(cr, "-60", font, xlp, ylp, 0, 2, c_wht);
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
	pango_layout_set_font_description(pl, ui->font[0]);
	pango_layout_set_text(pl, txt, -1);
	pango_layout_get_pixel_size(pl, &tw, &th);
	cairo_translate (cr, d->w_cx, d->w_height);
	cairo_translate (cr, -tw/2.0 - 0.5, -th);
	cairo_set_source_rgba (cr, .0, .0, .0, .7);
	rounded_rectangle(cr, -1, -1, tw+3, th+1, 3);
	cairo_fill(cr);
	CairoSetSouerceRGBA(c_wht);
	pango_cairo_show_layout(cr, pl);
	g_object_unref(pl);
	cairo_restore(cr);
	cairo_new_path(cr);
}

static void dial_annotation_val(RobTkDial * d, cairo_t *cr, void *data) {
	MixTriUI* ui = (MixTriUI*) (data);
	char tmp[16];
	switch(d->click_state) {
		case 1:
			snprintf(tmp, 16, "-\u221EdB");
			break;
		case 2:
			snprintf(tmp, 16, "\u00D8%+4.1fdB", d->cur);
			break;
		default:
			snprintf(tmp, 16, "%+4.1fdB", d->cur);
			break;
	}
	annotation_txt(ui, d, cr, tmp);
}

static void dial_annotation_db(RobTkDial * d, cairo_t *cr, void *data) {
	MixTriUI* ui = (MixTriUI*) (data);
	char tmp[16];
	snprintf(tmp, 16, "%+4.1fdB", d->cur);
	annotation_txt(ui, d, cr, tmp);
}

static bool box_expose_event(RobWidget* rw, cairo_t* cr, cairo_rectangle_t *ev) {
	if (rw->resized) {
		cairo_rectangle_t event;
		event.x = MAX(0, ev->x - rw->area.x);
		event.y = MAX(0, ev->y - rw->area.y);
		event.width  = MIN(rw->area.x + rw->area.width , ev->x + ev->width)   - MAX(ev->x, rw->area.x);
		event.height = MIN(rw->area.y + rw->area.height, ev->y + ev->height) - MAX(ev->y, rw->area.y);
		cairo_save(cr);
		rcontainer_clear_bg(rw, cr, &event);

		const float ytop = ((struct rob_table*)rw->self)->rows[0].acq_h;
		float tx0 = 0, tc0 = 0, tc1 = 0;
		for (uint32_t i = 0; i < 8; ++i) {
			tx0 += ((struct rob_table*)rw->self)->cols[i].acq_w;
			if (i == 0) tc0 = tx0;
			if (i == 3) tc1 = tx0;
		}
		const float yof = ytop + MIX_CY;
		const float twi = ((struct rob_table*)rw->self)->cols[8].acq_w;
		const float tx1 = tx0 + twi / 2;

		/* fixup in-delayline bg  & channel mods */
		cairo_set_source_rgba (cr, .4, .3, .3, 1.0);
		cairo_rectangle (cr, tc0, ytop, tc1-tc0, 160);
		cairo_fill(cr);

		/* right end trigger background */
		cairo_set_source_rgba (cr, .2, .3, .35, 1.0);
		cairo_rectangle (cr, tx0, ytop, twi, 160+30);
		cairo_fill(cr);

		cairo_set_line_width(cr, 1.0);
		CairoSetSouerceRGBA(c_g60);

		for (uint32_t i = 0; i < 4; ++i) {
			const float y0 = yof + i * MIX_HEIGHT;
			cairo_move_to(cr, tc0, y0);
			cairo_line_to(cr, tc1, y0);
			cairo_stroke(cr);
		}

		const double dashed[] = {2.5};
		cairo_set_dash(cr, dashed, 1, 4);
		for (uint32_t i = 0; i < 4; ++i) {
			const float y0 = yof + i * MIX_HEIGHT;
			cairo_move_to(cr, tx0-2, y0);
			cairo_line_to(cr, tx1, y0);
			cairo_stroke(cr);
		}
		cairo_set_dash(cr, NULL, 0, 0);
		CairoSetSouerceRGBA(c_blk);
		for (uint32_t i = 0; i < 5; ++i) {
			float y0 = yof + i * MIX_HEIGHT;
			cairo_move_to(cr, tx1+.5, y0);
			cairo_line_to(cr, tx1+.5, y0 + MIX_HEIGHT);
			cairo_stroke(cr);

			if (i == 4) y0 -= 10; // MIX_HEIGHT - GED_HEIGHT

			cairo_move_to(cr, tx1+.5-3, y0+23-6.5);
			cairo_line_to(cr, tx1+.5+3, y0+23-6.5);
			cairo_line_to(cr, tx1+.5,   y0+23-0.5);
			cairo_close_path(cr);
			cairo_fill(cr);
		}
		cairo_restore(cr);
	}
	return rcontainer_expose_event_no_clear(rw, cr, ev);
}

static void draw_arrow (cairo_t *cr, float x, float y, bool down) {
	cairo_save(cr);
	cairo_set_source_rgba (cr, .95, 1.0, .95, .8);
	cairo_set_line_width(cr, 1.0);
	cairo_move_to(cr, x+.5, y+.5);
	if (down) {
		cairo_line_to(cr, x+.5, y+12.5);
		cairo_stroke(cr);
		cairo_move_to(cr, x+.5, y+12.5);
		cairo_line_to(cr, x+3.5,  y+7.5);
		cairo_line_to(cr, x-2.5,  y+7.5);
		cairo_close_path(cr);
		cairo_fill(cr);
	} else {
		cairo_line_to(cr, x+.5, y-11.5);
		cairo_stroke(cr);
		cairo_move_to(cr, x+.5, y-11.5);
		cairo_line_to(cr, x+3.5,  y-6.5);
		cairo_line_to(cr, x-2.5,  y-6.5);
		cairo_close_path(cr);
		cairo_fill(cr);
	}
	cairo_restore(cr);
}

static void draw_cross (cairo_t *cr, float x, float y) {
	cairo_save(cr);
	cairo_set_source_rgba (cr, .95, 1.0, .95, .8);
	cairo_set_line_width(cr, 1.0);

	cairo_move_to(cr, x-2.5, y-2.5);
	cairo_line_to(cr, x+3.5, y+3.5);
	cairo_stroke(cr);
	cairo_move_to(cr, x+3.5, y-2.5);
	cairo_line_to(cr, x-2.5, y+3.5);
	cairo_stroke(cr);
	cairo_restore(cr);
}

static void draw_timedelta (cairo_t *cr, float x, float y, float w, float dw) {
	cairo_save(cr);
	cairo_set_line_width(cr, 1.0);

	if (dw > 0) {
		cairo_set_source_rgba (cr, .95, 1.0, .95, .6);
		cairo_rectangle(cr, x+w-dw+.5, y-2.5, dw*2, 6.0);
		cairo_fill(cr);
	}

	cairo_set_source_rgba (cr, .95, 1.0, .95, .8);
	cairo_move_to(cr, x+.5, y-2.5);
	cairo_line_to(cr, x+.5, y+3.5);
	cairo_stroke(cr);

	cairo_move_to(cr, x+.5, y+.5);
	cairo_line_to(cr, x+w+.5, y+.5);
	cairo_stroke(cr);

	cairo_move_to(cr, x+w+.5, y-1.5);
	cairo_line_to(cr, x+w+.5, y+2.5);
	cairo_stroke(cr);

	cairo_restore(cr);
}

#define ANN_TEXT(txt) \
	write_text_full(cr, txt, ui->font[1], 0, doc_h, 0, 6, c_wht);

static void draw_trigger_doc (cairo_t *cr, void *d) {
	MixTriUI* ui = (MixTriUI*) (d);
	int mode = robtk_select_get_value(ui->sel_trig_mode);
	int edge = robtk_select_get_value(ui->sel_trig_edge);

	float c_bg[4];
	get_color_from_theme(1, c_bg);

	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	CairoSetSouerceRGBA(c_bg);
	cairo_rectangle (cr, 0, 0, 150, 180);
	cairo_fill(cr);

	cairo_set_source_rgba (cr, .1, .1, .1, 1.0);
	cairo_rectangle (cr, 5, 0, 140, 180);
	cairo_fill(cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	cairo_save(cr);
	cairo_translate(cr, 10, 5);
	const float doc_w = 130;
	const float doc_h = 175;
	const float doc_b = 100;
	cairo_rectangle(cr, 0, 0, doc_w, doc_h);
	cairo_clip(cr);

	/* grid */

	cairo_set_line_width(cr, 1.5);
	cairo_set_source_rgba (cr, .4, .4, .4, 1.0);

	const double dash[] = {1.5};
	cairo_set_line_width(cr, 1.0);
	cairo_set_dash(cr, dash, 1, 0);

	switch (mode) {
		case TRG_PASSTRHU :
			break;
		default:
			for (uint32_t i = 5; i < doc_w; i+=10) {
				cairo_move_to(cr, i+.5, 0);
				cairo_line_to(cr, i+.5, doc_b);
				cairo_stroke(cr);
			}
			for (uint32_t i = 5; i < doc_b; i+=10) {
				cairo_move_to(cr, 0, i+.5);
				cairo_line_to(cr, doc_w, i+.5);
				cairo_stroke(cr);
			}
			break;
	}

	cairo_set_dash(cr, NULL, 0, 0);

	/* waveform */

	cairo_set_source_rgba (cr, 1.0, .0, .0, 1.0);

	switch (mode) {
		case TRG_EDGE :
		case TRG_WINDOW_ENTER:
		case TRG_WINDOW_LEAVE:
			cairo_move_to(cr, 5, 60);
			cairo_curve_to(cr, 15, 50, 20, 75, 25, 80);
			cairo_curve_to(cr, 25, 80, 38, 90, 45, 70);
			cairo_curve_to(cr, 45, 70, 60, 0, 87, 40);
			cairo_curve_to(cr, 88, 40, 100, 75, 120, 25);
			cairo_line_to(cr, 125, 8);
			cairo_stroke(cr);
			break;
		case TRG_DROPIN:
		case TRG_HYSTERESIS:
			cairo_move_to(cr, 5, 20);
			cairo_curve_to(cr, 18, 40, 22, 75, 30, 80);
			cairo_curve_to(cr, 30, 80, 38, 90, 45, 70);
			cairo_curve_to(cr, 45, 70, 60, 0, 87, 40);
			cairo_curve_to(cr, 88, 40, 100, 75, 120, 25);
			cairo_line_to(cr, 125, 8);
			cairo_stroke(cr);
			break;
		case TRG_DROPOUT:
			cairo_move_to(cr,  5.5, 75.5);
			cairo_line_to(cr, 25.5, 75.5);
			cairo_line_to(cr, 26.5, 25.5);
			cairo_line_to(cr, 55.5, 25.5);
			cairo_line_to(cr, 56.5, 75.5);
			cairo_line_to(cr,125.5, 75.5);
			cairo_stroke(cr);
			break;
		case TRG_RUNT:
			cairo_move_to(cr,  5.5, 75.5);
			cairo_line_to(cr, 15.5, 75.5);
			cairo_line_to(cr, 16.5, 25.5);
			cairo_line_to(cr, 35.5, 25.5);
			cairo_line_to(cr, 36.5, 75.5);
			cairo_line_to(cr, 55.5, 75.5);
			cairo_line_to(cr, 56.5, 45.5);
			cairo_line_to(cr, 75.5, 45.5);
			cairo_line_to(cr, 76.5, 75.5);
			cairo_line_to(cr, 95.5, 75.5);
			cairo_line_to(cr, 96.5, 25.5);
			cairo_line_to(cr,105.5, 25.5);
			cairo_line_to(cr,106.5, 55.5);
			cairo_line_to(cr,115.5, 55.5);
			cairo_line_to(cr,116.5, 25.5);
			cairo_line_to(cr,125.5, 25.5);
			cairo_stroke(cr);
			break;
		case TRG_PULSEWIDTH:
		case TRG_PULSETRAIN:
			cairo_move_to(cr,  5.5, 25.5);
			cairo_line_to(cr,  6.5, 75.5);
			cairo_line_to(cr, 25.5, 75.5);
			cairo_line_to(cr, 26.5, 25.5);
			cairo_line_to(cr, 45.5, 25.5);
			cairo_line_to(cr, 46.5, 75.5);
			cairo_line_to(cr, 75.5, 75.5);
			cairo_line_to(cr, 76.5, 25.5);
			cairo_line_to(cr, 85.5, 25.5);
			cairo_line_to(cr, 86.5, 75.5);
			cairo_line_to(cr, 95.5, 75.5);
			cairo_line_to(cr, 96.5, 25.5);
			cairo_line_to(cr,115.5, 25.5);
			cairo_line_to(cr,116.5, 75.5);
			cairo_line_to(cr,125.5, 75.5);
			cairo_stroke(cr);
			break;
		case TRG_LTC:
			cairo_move_to(cr,  5.5, 75.5);
			for (int i = 0; i < 70; i+=5) {
				cairo_line_to(cr, i + 10.5, (i%10 != 0)? 25.5 : 75.5);
				cairo_line_to(cr, i + 11.5, (i%10 == 0)? 25.5 : 75.5);
			}
			cairo_line_to(cr,85.5, 75.5);
			cairo_line_to(cr,86.5, 25.5);
			cairo_line_to(cr,90.5, 25.5);
			cairo_line_to(cr,91.5, 75.5);
			cairo_line_to(cr,95.5, 75.5);
			cairo_line_to(cr,96.5, 25.5);
			cairo_line_to(cr,105.5, 25.5);
			cairo_line_to(cr,106.5, 75.5);
			cairo_line_to(cr,115.5, 75.5);
			cairo_line_to(cr,116.5, 25.5);
			cairo_line_to(cr,120.5, 25.5);
			cairo_line_to(cr,121.5, 75.5);
			cairo_line_to(cr,125.5, 75.5);
			cairo_line_to(cr,126.5, 25.5);
			cairo_stroke(cr);
			break;
		case TRG_LPF:
			cairo_move_to(cr,  5.5, 50);
			{
				float phase = 0;
				for (uint32_t i = 1; i < 120; ++i) {
					phase+=.05 + .4 * i / 120.0;
					cairo_line_to(cr, i + 5.5, 50 + 40.0 * sinf(phase));
				}
			}
			cairo_stroke(cr);
			break;
		case TRG_RMS:
			cairo_move_to(cr,  5.5, 50);
			for (uint32_t i = 1; i < 120; ++i) {
				cairo_line_to(cr, i + 5.5, 50 + 40.0 * sinf(i * .2));
			}
			cairo_stroke(cr);
			break;
		case TRG_PASSTRHU:
		default:
			break;
	}

	/* settings & annotation */
	switch (mode) {
		case TRG_EDGE :
			cairo_set_source_rgba (cr, .0, 1.0, .0, .8);
			cairo_move_to(cr, 0, 40.5);
			cairo_line_to(cr, doc_w, 40.5);
			cairo_stroke(cr);
			if (edge&1) {
				draw_arrow(cr, 55, 70, false);
				draw_arrow(cr, 112, 70, false);
			}
			if (edge&2) {
				draw_arrow(cr, 85, 10, true);
			}
			ANN_TEXT("Signal Edge\n Signal passes 'Level 1'.");
			break;
		case TRG_PULSEWIDTH:
			cairo_set_source_rgba (cr, .0, 1.0, .0, .8);
			cairo_move_to(cr, 0, 40.5);
			cairo_line_to(cr, doc_w, 40.5);
			cairo_stroke(cr);

			if (edge & 1) {
				draw_cross(cr, 25, 40);
				draw_cross(cr, 75, 40);
				draw_cross(cr, 95, 40);
			}

			if (edge & 2) {
				draw_cross(cr,  5, 40);
				draw_cross(cr, 45, 40);
				draw_cross(cr, 85, 40);
				draw_cross(cr,115, 40);
			}

			switch (edge) {
				case 1:
					draw_timedelta(cr, 25, 90, 20, 5);
					draw_timedelta(cr, 75, 90, 20, 5);
					draw_arrow(cr,95, 20, false);
					break;
				case 2:
					draw_timedelta(cr,  5, 80, 40, 5);
					draw_timedelta(cr, 45, 90, 40, 5);
					draw_arrow(cr, 45, 10, true);
					draw_arrow(cr, 85, 10, true);
					break;
				case 3:
					draw_timedelta(cr,  5, 80, 20, 5);
					draw_timedelta(cr, 25, 90, 20, 5);
					draw_timedelta(cr, 45, 80, 20, 5);
					draw_timedelta(cr, 75, 90, 20, 5);
					draw_timedelta(cr, 85, 90, 20, 5);
					draw_timedelta(cr, 95, 80, 20, 5);

					draw_arrow(cr, 25, 20, false);
					draw_arrow(cr, 45, 10, true);
					draw_arrow(cr,115, 10, true);
					break;
			}

			ANN_TEXT("Pulse Width\n Last edge-trigger\n occurred between min\n and max (Time 1, 2) ago.");
			break;
		case TRG_PULSETRAIN:
			cairo_set_source_rgba (cr, .0, 1.0, .0, .8);
			cairo_move_to(cr, 0, 40.5);
			cairo_line_to(cr, doc_w, 40.5);
			cairo_stroke(cr);

			if (edge & 1) {
				draw_cross(cr, 25, 40);
				draw_cross(cr, 75, 40);
				draw_cross(cr, 95, 40);
			}
			if (edge & 2) {
				draw_cross(cr,  5, 40);
				draw_cross(cr, 45, 40);
				draw_cross(cr, 85, 40);
				draw_cross(cr,115, 40);
			}

			switch (edge) {
				case 1:
					draw_timedelta(cr, 25, 90, 20, 5);
					draw_timedelta(cr, 75, 90, 20, 5);
					draw_timedelta(cr, 95, 80, 20, 5);
					draw_arrow(cr, 50, 20, false);
					draw_arrow(cr,120, 20, false);
					break;
				case 2:
					draw_timedelta(cr,  5, 80, 40, 5);
					draw_timedelta(cr, 45, 90, 40, 5);
					draw_timedelta(cr, 85, 80, 40, 5);
					draw_arrow(cr,115, 10, true);
					break;
				case 3:
					draw_timedelta(cr,  5, 80, 20, 5);
					draw_timedelta(cr, 25, 90, 20, 5);
					draw_timedelta(cr, 45, 80, 20, 5);
					draw_timedelta(cr, 75, 90, 20, 5);

					draw_arrow(cr, 70, 10, true);
					draw_arrow(cr, 85, 20, false);
					break;
			}

			ANN_TEXT("Pulse Train\n No edge-trigger for a\n given time (max, Time 2),\n or more than one trigger\n since a given time (min,\n Time 1).");
			break;
		case TRG_WINDOW_ENTER:
			cairo_set_source_rgba (cr, .0, 1.0, .0, .8);
			cairo_move_to(cr, 0, 35.5);
			cairo_line_to(cr, doc_w, 35.5);
			cairo_stroke(cr);
			cairo_move_to(cr, 0, 45.5);
			cairo_line_to(cr, doc_w, 45.5);
			cairo_stroke(cr);


			if (edge&1) {
				cairo_set_source_rgba (cr, .0, 1.0, .0, .4);
				cairo_rectangle(cr,  53, 35.5, 5, 10); cairo_fill(cr);
				cairo_rectangle(cr, 108, 35.5, 7, 10); cairo_fill(cr);
				draw_arrow(cr, 53, 70, false);
				draw_arrow(cr,108, 70, false);
			}
			if (edge&2) {
				cairo_set_source_rgba (cr, .0, 1.0, .0, .4);
				cairo_rectangle(cr,  84, 35.5, 6, 10); cairo_fill(cr);
				draw_arrow(cr, 84, 10, true);
			}
			ANN_TEXT("Enter Window\n Signal enters a given\n range (Level 1, 2).");
			break;
		case TRG_WINDOW_LEAVE:
			cairo_set_source_rgba (cr, .0, 1.0, .0, .8);
			cairo_move_to(cr, 0, 35.5);
			cairo_line_to(cr, doc_w, 35.5);
			cairo_stroke(cr);
			cairo_move_to(cr, 0, 45.5);
			cairo_line_to(cr, doc_w, 45.5);
			cairo_stroke(cr);

			cairo_set_source_rgba (cr, .0, 1.0, .0, .4);
			if (edge&1) {
				cairo_rectangle(cr,  53, 35.5, 5, 10); cairo_fill(cr);
				cairo_rectangle(cr, 108, 35.5, 7, 10); cairo_fill(cr);
				draw_arrow(cr, 58, 70, false);
				draw_arrow(cr,115, 70, false);
			}
			if (edge&2) {
				cairo_rectangle(cr,  84, 35.5, 6, 10); cairo_fill(cr);
				draw_arrow(cr, 90, 10, true);
			}

			ANN_TEXT("Leave Window\n Signal leaves a given\n range (Level 1, 2).");
			break;
		case TRG_HYSTERESIS:
			cairo_set_source_rgba (cr, .0, 1.0, .0, .8);
			cairo_move_to(cr, 0, 35.5);
			cairo_line_to(cr, doc_w, 35.5);
			cairo_stroke(cr);
			cairo_move_to(cr, 0, 65.5);
			cairo_line_to(cr, doc_w, 65.5);
			cairo_stroke(cr);
			if (edge&1) {
				draw_arrow(cr, 58, 80, false);
			}
			if (edge&2) {
				draw_arrow(cr, 23, 10, true);
			}
			ANN_TEXT("Hysteresis\n Signal crosses both min\n and max (Level 1, 2) in\n the same direction\n without interruption.");
			break;
		case TRG_RUNT:
			cairo_set_source_rgba (cr, .0, 1.0, .0, .8);
			cairo_move_to(cr, 0, 40.5);
			cairo_line_to(cr, doc_w, 40.5);
			cairo_stroke(cr);
			cairo_move_to(cr, 0, 60.5);
			cairo_line_to(cr, doc_w, 60.5);
			cairo_stroke(cr);
			if (edge & 1) {
				draw_arrow(cr, 75, 90, false);
			}
			if (edge & 2) {
				draw_arrow(cr, 115, 10, true);
			}
			ANN_TEXT("Runt\n Fire if signal crosses 1st,\n but not 2nd threshold.");
			break;
		case TRG_DROPOUT:
			cairo_set_source_rgba (cr, .0, 1.0, .0, .8);
			cairo_move_to(cr, 0, 40.5);
			cairo_line_to(cr, doc_w, 40.5);
			cairo_stroke(cr);
			cairo_move_to(cr, 0, 60.5);
			cairo_line_to(cr, doc_w, 60.5);
			cairo_stroke(cr);

			cairo_set_source_rgba (cr, .0, 1.0, .0, .2);
			cairo_set_line_width(cr, 4.0);
			cairo_move_to(cr, 25.5,     40.5);
			cairo_line_to(cr, 25.5, 60.5);
			cairo_stroke(cr);
			cairo_move_to(cr, 55.5,     40.5);
			cairo_line_to(cr, 55.5, 60.5);
			cairo_stroke(cr);

			if (edge & 1) {
				draw_cross(cr,  25, 50);
				draw_timedelta(cr, 25, 80, 40, 0);
			}
			if (edge & 2) {
				draw_cross(cr,  55, 50);
				draw_timedelta(cr, 55, 90, 40, 0);
			}
			if (edge == 1) {
				draw_arrow(cr, 65, 100, false);
			} else {
				draw_arrow(cr, 95, 10, true);
			}

			ANN_TEXT("Dropout\n Signal does not pass\n though a given range\n for at least 'Time 1'.");
			break;
		case TRG_DROPIN:
			cairo_set_source_rgba (cr, .0, 1.0, .0, .8);
			cairo_move_to(cr, 0, 65.5);
			cairo_line_to(cr, doc_w, 65.5);
			cairo_stroke(cr);
			cairo_move_to(cr, 0, 25.5);
			cairo_line_to(cr, doc_w, 25.5);
			cairo_stroke(cr);

			cairo_set_source_rgba (cr, .0, 1.0, .0, .2);
			switch(edge) {
				case 1:
					cairo_rectangle(cr, 48, 25.5, 70, 40);
					cairo_fill(cr);
					draw_timedelta(cr, 47, 65, 40, 0);
					draw_arrow(cr, 87, 90, false);
					break;
				case 2:
					cairo_rectangle(cr, 8, 25.5, 15, 40);
					cairo_fill(cr);
					draw_timedelta(cr, 8, 65, 10, 0);
					draw_arrow(cr, 18, 10, true);
					break;
				case 3:
					cairo_rectangle(cr, 8, 25.5, 15, 40);
					cairo_fill(cr);
					cairo_rectangle(cr, 48, 25.5, 70, 40);
					cairo_fill(cr);
					draw_timedelta(cr, 47, 65, 10, 0);
					draw_timedelta(cr, 8, 65, 10, 0);
					draw_arrow(cr, 18, 10, true);
					draw_arrow(cr, 57, 90, false);
					break;
			}

			ANN_TEXT("Constrained\n Signal remains within a\n given range for at least\n 'Time 1'.");
			break;
		case TRG_RMS:
			cairo_set_source_rgba (cr, .0, 0.5, 1.0, .8);
			cairo_move_to(cr,  5.5, 50);
			{
				float ts_prev = 0;
				for (uint32_t i = 1; i < 120; ++i) {
					const float y0 = sinf(i * .2);
					ts_prev += .02 * (y0 * y0 - ts_prev);
					cairo_line_to(cr, i + 5.5, 50 - 40.0 * sqrtf(ts_prev));
				}
			}
			cairo_stroke(cr);
			ANN_TEXT("Calculate RMS\n time constant\n 'Time 1'");
			break;
		case TRG_LPF:
			cairo_set_source_rgba (cr, .0, 0.5, 1.0, .8);
			cairo_move_to(cr,  5.5, 50);
			{
				float ts_prev = 0;
				float phase = 0;
				for (uint32_t i = 1; i < 120; ++i) {
					phase+=.05 + .4 * i / 120.0;
					const float y0 = sinf(phase);
					ts_prev += .1 * (y0 - ts_prev);
					cairo_line_to(cr, i + 5.5, 50 + 50.0 * ts_prev);
				}
			}
			cairo_stroke(cr);
			ANN_TEXT("Low Pass Filter\n 1.0 / 'Time 1' Hz");
			break;
		case TRG_PASSTRHU:
			ANN_TEXT("Signal Passthough");
			break;
		case TRG_LTC:
			draw_arrow(cr, 95, 90, false);
			ANN_TEXT("Linear Time Code\n LTC sync word.");
			break;
		default:
			break;
	}

	cairo_restore(cr);
}

static bool cb_show_doc (RobWidget* handle, void *d) {
	MixTriUI* ui = (MixTriUI*) (d);
	if (robtk_cbtn_get_active(ui->btn_show_doc)) {
		robwidget_show(ui->drawing_area->rw, true);
	} else {
		robwidget_hide(ui->drawing_area->rw, true);
	}
	return TRUE;
}

/******************************************************************************
 * UI callbacks
 */

#define TRIGGERSENS(ED,L0,L1,T0,T1) \
	robtk_select_set_sensitive(ui->sel_trig_edge,    (ED)?true:false); \
	robtk_spin_set_sensitive(ui->spb_trigger_lvl[0], (L0)?true:false); \
	robtk_spin_set_sensitive(ui->spb_trigger_lvl[1], (L1)?true:false); \
	robtk_spin_set_sensitive(ui->spb_trigger_tme[0], (T0)?true:false); \
	robtk_spin_set_sensitive(ui->spb_trigger_tme[1], (T1)?true:false);

static bool cb_set_trig_mode (RobWidget* handle, void *data) {
	MixTriUI* ui = (MixTriUI*) (data);
	float mode = robtk_select_get_value(ui->sel_trig_mode);
	switch ((int)mode) {
		case TRG_PASSTRHU:
		case TRG_LTC:
			TRIGGERSENS(0,0,0,0,0);
			break;
		case TRG_PULSEWIDTH:
		case TRG_PULSETRAIN:
			TRIGGERSENS(1,1,0,1,1);
			break;
		case TRG_WINDOW_ENTER:
		case TRG_WINDOW_LEAVE:
		case TRG_RUNT:
			TRIGGERSENS(1,1,1,0,0);
			break;
		case TRG_HYSTERESIS:
			TRIGGERSENS(1,1,1,0,0);
			break;
		case TRG_DROPIN:
		case TRG_DROPOUT:
			TRIGGERSENS(1,1,1,1,0);
			break;
		case TRG_RMS:
		case TRG_LPF:
			TRIGGERSENS(0,0,0,1,0);
			break;
		case TRG_EDGE :
			TRIGGERSENS(1,1,0,0,0);
			break;
	}
	if (robtk_cbtn_get_active(ui->btn_show_doc)) {
		robtk_darea_redraw(ui->drawing_area);
	}
	if (ui->disable_signals) return TRUE;
	ui->write(ui->controller, MIXTRI_TRIG_MODE, sizeof(float), 0, (const void*) &mode);
	return TRUE;
}

static bool cb_set_trig_edge (RobWidget* handle, void *data) {
	MixTriUI* ui = (MixTriUI*) (data);
	if (robtk_cbtn_get_active(ui->btn_show_doc)) {
		robtk_darea_redraw(ui->drawing_area);
	}
	if (ui->disable_signals) return TRUE;
	float mode = robtk_select_get_value(ui->sel_trig_edge);
	ui->write(ui->controller, MIXTRI_TRIG_EDGE, sizeof(float), 0, (const void*) &mode);
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

static bool cb_set_trig_values (RobWidget* handle, void *data) {
	MixTriUI* ui = (MixTriUI*) (data);
	float val0, val1;
	if (ui->disable_signals) return TRUE;

	val0 = robtk_spin_get_value(ui->spb_trigger_lvl[0]);
	val1 = robtk_spin_get_value(ui->spb_trigger_lvl[1]);
	if (val1 < val0) {
		ui->disable_signals = true;
		robtk_spin_set_value(ui->spb_trigger_lvl[1], val0);
		val1 = val0;
	}
	ui->write(ui->controller, MIXTRI_TRIG_LVL1, sizeof(float), 0, (const void*) &val1);
	ui->write(ui->controller, MIXTRI_TRIG_LVL0, sizeof(float), 0, (const void*) &val0);
	val0 = robtk_spin_get_value(ui->spb_trigger_tme[0]);
	val1 = robtk_spin_get_value(ui->spb_trigger_tme[1]);
	if (val1 < val0) {
		ui->disable_signals = true;
		robtk_spin_set_value(ui->spb_trigger_tme[1], val0);
		val1 = val0;
	}
	ui->write(ui->controller, MIXTRI_TRIG_TME1, sizeof(float), 0, (const void*) &val1);
	ui->write(ui->controller, MIXTRI_TRIG_TME0, sizeof(float), 0, (const void*) &val0);
	ui->disable_signals = false; // XXX
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
		float val = pow(10, .05 * robtk_dial_get_value(ui->dial_mix[i]));
		switch(robtk_dial_get_state(ui->dial_mix[i])) {
			case 1:
				val = 0;
				break;
			case 2:
				val *= -1;
				break;
			default:
				break;
		}
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

static RobWidget *toplevel_mixtri(MixTriUI* ui)
{
	ui->hbox = rob_hbox_new(FALSE, 2);

	ui->font[0] = pango_font_description_from_string("Mono 10px");
	ui->font[1] = pango_font_description_from_string("Sans 9px");
	create_faceplate(ui);

	ui->label[0] = robtk_lbl_new("Delay [spl]");
	ui->label[1] = robtk_lbl_new("Output Delay [spl] \u2192 ");
	ui->label[2] = robtk_lbl_new("Mixer Matrix [amp]");
	ui->label[3] = robtk_lbl_new("Channel mod.");
	ui->label[4] = robtk_lbl_new("Out Trigger");
	ui->label[5] = robtk_lbl_new("Gain");
	ui->label[6] = robtk_lbl_new("Trigger");
	// TODO use robtk_info()
#ifdef MIXTRILV2
	ui->label[7] = robtk_lbl_new("x42 MixTri LV2 " VERSION);
#else
	ui->label[7] = robtk_lbl_new("");
#endif
	ui->label[8] = robtk_lbl_new("Trig. Settings");

	robtk_lbl_set_alignment(ui->label[0], 0.5, 0.5);
	robtk_lbl_set_alignment(ui->label[1], 1.0, 0.25);
	robtk_lbl_set_alignment(ui->label[2], 0.5, 0.5);
	robtk_lbl_set_alignment(ui->label[3], 0.5, 0.5);
	robtk_lbl_set_alignment(ui->label[4], 0.5, 0.5);
	robtk_lbl_set_alignment(ui->label[5], 0.5, 0.5);
	robtk_lbl_set_alignment(ui->label[6], 0.5, 0.5);
	robtk_lbl_set_alignment(ui->label[7], 0.0, 0.5);
	robtk_lbl_set_alignment(ui->label[8], 0.5, 0.5);

	robtk_lbl_set_color(ui->label[7], .6, .6, .6, 1.0);

	ui->ctable = rob_table_new(/*rows*/7, /*cols*/ 9, FALSE);
	ui->ctable->expose_event = box_expose_event;

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
	rob_table_attach(ui->ctable, robtk_lbl_widget(ui->label[8]),
			10, 12, 0, 1, 0, 0, RTK_EXANDF, RTK_SHRINK);


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
		ui->dial_mix[i] = robtk_dial_new_with_size(-20.0, 20.0, .01,
				MIX_WIDTH, MIX_HEIGHT, MIX_CX, MIX_CY, MIX_RADIUS);
		const int g = ((i%3) == (i/3)) ? 0 : 1;
		robtk_dial_enable_states(ui->dial_mix[i], 2);
		robtk_dial_set_state_color(ui->dial_mix[i], 1, .15, .15, .15, 1.0);
		robtk_dial_set_state_color(ui->dial_mix[i], 2, 1.0, .0, .0, .3);
		robtk_dial_set_default(ui->dial_mix[i], 0);
		robtk_dial_set_default_state(ui->dial_mix[i], g);
		robtk_dial_set_state(ui->dial_mix[i], g);
		robtk_dial_set_value(ui->dial_mix[i], 0);
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
		ui->spb_delay_in[i]->dial->displaymode = 3;
		robtk_spin_set_default(ui->spb_delay_in[i], 0);
		robtk_spin_set_value(ui->spb_delay_in[i], 0);
		robtk_spin_set_callback(ui->spb_delay_in[i], cb_set_delay, ui);
		robtk_spin_label_width(ui->spb_delay_in[i], -1, MIX_WIDTH - GSP_WIDTH - 8);

		robtk_dial_set_surface(ui->spb_delay_in[i]->dial,ui->delayI);
		robtk_lbl_set_alignment(ui->spb_delay_in[i]->lbl_r, 0, 0.3);
		robtk_dial_set_alignment(ui->spb_delay_in[i]->dial, .5, 1.0);
		ui->spb_delay_in[i]->rw->yalign = .45;

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
		ui->spb_delay_out[i]->dial->displaymode = 3;
		robtk_spin_set_callback(ui->spb_delay_out[i], cb_set_delay, ui);
		robtk_spin_set_default(ui->spb_delay_out[i], 0);
		robtk_spin_set_value(ui->spb_delay_out[i], 0);
		robtk_dial_set_surface(ui->spb_delay_out[i]->dial,ui->delayO);
		robtk_lbl_set_alignment(ui->spb_delay_out[i]->lbl_r, 0, 0.3);
		robtk_spin_label_width(ui->spb_delay_out[i], -1, MIX_WIDTH - GSP_WIDTH - 8);
		rob_table_attach(ui->ctable, robtk_spin_widget(ui->spb_delay_out[i]),
				i+5, i+6, 5, 6, 0, 0, RTK_EXANDF, RTK_SHRINK);
	}

	ui->sel_trig_mode = robtk_select_new();
	robtk_select_set_alignment(ui->sel_trig_mode, .5, 0);
	ui->sel_trig_mode->t_width = MIX_WIDTH - 36;
	robtk_select_add_item(ui->sel_trig_mode, TRG_PASSTRHU, "-");
	robtk_select_add_item(ui->sel_trig_mode, TRG_EDGE, "Edge");
	robtk_select_add_item(ui->sel_trig_mode, TRG_WINDOW_ENTER, "Enter Window");
	robtk_select_add_item(ui->sel_trig_mode, TRG_WINDOW_LEAVE, "Leave Window");
	robtk_select_add_item(ui->sel_trig_mode, TRG_HYSTERESIS, "Hysteresis");
	robtk_select_add_item(ui->sel_trig_mode, TRG_DROPIN, "Constrained");
	robtk_select_add_item(ui->sel_trig_mode, TRG_DROPOUT, "Dropout");
	robtk_select_add_item(ui->sel_trig_mode, TRG_PULSEWIDTH, "Pulse Width");
	robtk_select_add_item(ui->sel_trig_mode, TRG_PULSETRAIN, "Pulse Train");
	robtk_select_add_item(ui->sel_trig_mode, TRG_RUNT, "Runt");
	robtk_select_add_item(ui->sel_trig_mode, TRG_LTC, "LTC");
	robtk_select_add_item(ui->sel_trig_mode, TRG_RMS, "RMS");
	robtk_select_add_item(ui->sel_trig_mode, TRG_LPF, "LPF");
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

#define TBLADD(WIDGET, C0, C1, R) \
	rob_table_attach(ui->ctable, WIDGET, \
			(C0), (C1), (R), (R)+1, 0, 0, RTK_EXANDF, RTK_SHRINK)

	/* trigger settings */
	ui->lbl_trig[0] = robtk_lbl_new("Level 1: ");
	ui->lbl_trig[1] = robtk_lbl_new("Level 2: ");
	ui->lbl_trig[2] = robtk_lbl_new("Time 1: ");
	ui->lbl_trig[3] = robtk_lbl_new("Time 2: ");

	for (uint32_t i = 0; i < 4; ++i) {
		robtk_lbl_set_alignment(ui->lbl_trig[i], 1.0, 0.5);
		TBLADD(robtk_lbl_widget(ui->lbl_trig[i]), 10, 11, 2+i);
	}

	ui->sel_trig_edge = robtk_select_new();
	robtk_select_add_item(ui->sel_trig_edge, 1, "rising edge");
	robtk_select_add_item(ui->sel_trig_edge, 2, "falling edge");
	robtk_select_add_item(ui->sel_trig_edge, 3, "any edge");
	robtk_select_set_callback(ui->sel_trig_edge, cb_set_trig_edge, ui);
	TBLADD(robtk_select_widget(ui->sel_trig_edge), 10, 12, 1);

	for (uint32_t i = 0; i < 2; ++i) {
		ui->spb_trigger_lvl[i] = robtk_spin_new(-1.f, 1.f, .01);
		robtk_spin_set_default(ui->spb_trigger_lvl[i], 0);
		robtk_spin_set_value(ui->spb_trigger_lvl[i], 0);
		robtk_spin_label_width(ui->spb_trigger_lvl[i], -1, 40); // XXX
		robtk_spin_set_alignment(ui->spb_trigger_lvl[i], 0, .5);
		robtk_spin_set_callback(ui->spb_trigger_lvl[i], cb_set_trig_values, ui);
		TBLADD(robtk_spin_widget(ui->spb_trigger_lvl[i]), 11, 12, 2+i);

		ui->spb_trigger_tme[i] = robtk_spin_new(0, MAXDELAY-1, 1);
		ui->spb_trigger_tme[i]->dial->displaymode = 3;
		robtk_spin_set_default(ui->spb_trigger_tme[i], 0);
		robtk_spin_set_value(ui->spb_trigger_tme[i], 0);
		robtk_spin_label_width(ui->spb_trigger_tme[i], -1, 50); // XXX
		robtk_spin_set_alignment(ui->spb_trigger_tme[i], 0, .5);
		robtk_spin_set_callback(ui->spb_trigger_tme[i], cb_set_trig_values, ui);
		TBLADD(robtk_spin_widget(ui->spb_trigger_tme[i]), 11, 12, 4+i);
	}

	ui->btn_show_doc = robtk_cbtn_new("Show Doc", GBT_LED_LEFT, true);
	robtk_cbtn_set_alignment(ui->btn_show_doc, 0, 0.5);
	robtk_cbtn_set_color_on(ui->btn_show_doc,  .8, .8, .9);
	robtk_cbtn_set_color_off(ui->btn_show_doc, .1, .1, .3);
	robtk_cbtn_set_alignment(ui->btn_show_doc, .5, .5);
	robtk_cbtn_set_active(ui->btn_show_doc, false);
	robtk_cbtn_set_callback(ui->btn_show_doc, cb_show_doc, ui);
	rob_table_attach(ui->ctable, robtk_cbtn_widget(ui->btn_show_doc),
			10, 12, 6, 7, 0, 0, RTK_EXANDF, RTK_SHRINK);

	ui->drawing_area = robtk_darea_new(150, 180, draw_trigger_doc, ui);
	robwidget_hide(ui->drawing_area->rw, false);
	rob_table_attach(ui->ctable, robtk_darea_widget(ui->drawing_area),
			12, 13, 1, 6, 0, 0, RTK_EXANDF, RTK_SHRINK);

	robtk_select_set_active_item(ui->sel_trig_edge, 0);
	cb_set_trig_mode(NULL, ui);

	rob_hbox_child_pack(ui->hbox, ui->ctable, FALSE, FALSE);
	return ui->hbox;
#undef TBLADD
}

/******************************************************************************
 * LV2
 */

static void
cleanup_mixtri(LV2UI_Handle handle)
{
	MixTriUI* ui = (MixTriUI*)handle;

	for (uint32_t i = 0; i < 12; ++i) {
		robtk_dial_destroy(ui->dial_mix[i]);
	}
	for (uint32_t i = 0; i < 4; ++i) {
		robtk_dial_destroy(ui->dial_in[i]);
		robtk_spin_destroy(ui->spb_delay_in[i]);
		robtk_cbtn_destroy(ui->btn_hpfilt_in[i]);
		robtk_cbtn_destroy(ui->btn_mute_in[i]);
		robtk_lbl_destroy(ui->lbl_in[i]);
		robtk_rbtn_destroy(ui->btn_trig_src[i]);
		robtk_lbl_destroy(ui->lbl_trig[i]);
	}
	for (uint32_t i = 0; i < 3; ++i) {
		robtk_spin_destroy(ui->spb_delay_out[i]);
		robtk_lbl_destroy(ui->lbl_out[i]);
	}
	for (uint32_t i = 0; i < 9; ++i) {
		robtk_lbl_destroy(ui->label[i]);
	}
	for (uint32_t i = 0; i < 2; ++i) {
		robtk_spin_destroy(ui->spb_trigger_lvl[i]);
		robtk_spin_destroy(ui->spb_trigger_tme[i]);
	}
	robtk_select_destroy(ui->sel_trig_mode);
	robtk_select_destroy(ui->sel_trig_edge);
	cairo_surface_destroy(ui->routeT);
	cairo_surface_destroy(ui->routeC);
	cairo_surface_destroy(ui->routeE);
	cairo_surface_destroy(ui->routeM);
	cairo_surface_destroy(ui->routeI);
	cairo_surface_destroy(ui->delayI);
	cairo_surface_destroy(ui->delayO);
	pango_font_description_free(ui->font[0]);
	pango_font_description_free(ui->font[1]);

	robtk_cbtn_destroy(ui->btn_show_doc);
	robtk_darea_destroy(ui->drawing_area);

	rob_box_destroy(ui->ctable);
	rob_box_destroy(ui->hbox);

	free(ui);
}

static void
port_event_mixtri(
		LV2UI_Handle handle,
		uint32_t     port,
		uint32_t     buffer_size,
		uint32_t     format,
		const void*  buffer)
{
	MixTriUI* ui = (MixTriUI*)handle;

	if (format != 0) return;
	const float v = *(float *)buffer;
	const int vi = *(float *)buffer;
	if (port >= MIXTRI_MIX_0_0 && port <= MIXTRI_MIX_3_2) {
		const int d = port - MIXTRI_MIX_0_0;
		ui->disable_signals = true;
		if (v == 0) {
			robtk_dial_set_state(ui->dial_mix[d], 1);
		} else if (v < 0) {
			robtk_dial_set_state(ui->dial_mix[d], 2);
			robtk_dial_set_value(ui->dial_mix[d], 20 * log10f(-v));
		} else {
			robtk_dial_set_state(ui->dial_mix[d], 0);
			robtk_dial_set_value(ui->dial_mix[d], 20 * log10f(v));
		}
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
	else if (port >= MIXTRI_TRIG_CHN && port <= MIXTRI_TRIG_TME1) {
		ui->disable_signals = true;
		switch(port) {
			case MIXTRI_TRIG_CHN:
				if (vi >=0 && vi < 4) robtk_rbtn_set_active(ui->btn_trig_src[vi], true);
				break;
			case MIXTRI_TRIG_MODE:
				robtk_select_set_value(ui->sel_trig_mode, vi);
				break;
			case MIXTRI_TRIG_EDGE:
				robtk_select_set_value(ui->sel_trig_edge, vi);
				break;
			case MIXTRI_TRIG_LVL0:
				robtk_spin_set_value(ui->spb_trigger_lvl[0], v);
				break;
			case MIXTRI_TRIG_LVL1:
				robtk_spin_set_value(ui->spb_trigger_lvl[1], v);
				break;
			case MIXTRI_TRIG_TME0:
				robtk_spin_set_value(ui->spb_trigger_tme[0], v);
				break;
			case MIXTRI_TRIG_TME1:
				robtk_spin_set_value(ui->spb_trigger_tme[1], v);
				break;
		}
		ui->disable_signals = false;
	}
}

#ifdef MIXTRILV2

#define RTK_URI MIXTRI_URI
#define RTK_GUI "ui"

/**
 * standalone robtk GUI
 */
static void ui_disable(LV2UI_Handle handle) { }
static void ui_enable(LV2UI_Handle handle) { }

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

	*widget = toplevel_mixtri(ui);
	robwidget_make_toplevel(ui->hbox, ui_toplevel);
	ROBWIDGET_SETNAME(ui->hbox, "mixtri");

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
	cleanup_mixtri(handle);
}

static void
port_event(
		LV2UI_Handle handle,
		uint32_t     port,
		uint32_t     buffer_size,
		uint32_t     format,
		const void*  buffer)
{
	port_event_mixtri(handle, port, buffer_size, format, buffer);
}

static const void*
extension_data(const char* uri)
{
	return NULL;
}
#endif
/* vi:set ts=2 sts=2 sw=2: */
