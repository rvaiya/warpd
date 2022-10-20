/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */
#include "wayland.h"

static char bgcolor[16];
static char fgcolor[16];
static const char *font_family;

static int calculate_font_size(cairo_t *cr, int w, int h)
{
	cairo_text_extents_t extents;
	size_t sz = 100;

	cairo_select_font_face (cr,
				font_family,
				CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

	do {
		cairo_set_font_size(cr, sz);
		cairo_text_extents(cr, "WW", &extents);
		sz--;
	} while (extents.height > h || extents.width > w);

	return sz;
}

static void cairo_draw_text(cairo_t *cr, const char *s, int x, int y, int w, int h)
{
	int ptsz = calculate_font_size(cr, w, h);

	cairo_select_font_face (cr,
				font_family,
				CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

	cairo_text_extents_t extents;
	cairo_set_font_size(cr, ptsz);

	cairo_text_extents(cr, s, &extents);

	cairo_move_to(cr, x + (w-extents.width)/2, y-extents.y_bearing + (h-extents.height)/2);
	cairo_show_text(cr, s);
}

void wl_hint_draw(struct screen *scr, struct hint *hints, size_t n) 
{
	size_t i;
	uint8_t r,g,b,a;

	cairo_t *cr = scr->overlay->cr;

	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(cr, 0, 0, 0, 0);
	cairo_paint(cr);

	for (i = 0; i < n; i++) {
		wl_hex_to_rgba(bgcolor, &r, &g, &b, &a);
		cairo_set_source_rgba(cr, r / 255.0, g / 255.0, b / 255.0,
				      a / 255.0);
		cairo_rectangle(cr, hints[i].x, hints[i].y, hints[i].w,
				hints[i].h);
		cairo_fill(cr);

		wl_hex_to_rgba(fgcolor, &r, &g, &b, &a);
		cairo_set_source_rgba(cr, r / 255.0, g / 255.0, b / 255.0,
				      a / 255.0);

		cairo_draw_text(cr, hints[i].label, hints[i].x, hints[i].y,
				hints[i].w, hints[i].h);
	}

	surface_show(scr->overlay, scr->wl_output);
}

void wl_init_hint(const char *bg, const char *fg, int border_radius, const char *font)
{
	strncpy(bgcolor, bg, sizeof bgcolor);
	strncpy(fgcolor, fg, sizeof fgcolor);

	//TODO: handle border radius

	font_family = font;
}
