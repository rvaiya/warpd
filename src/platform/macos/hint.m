#include "macos.h"

static float border_radius;

static NSColor *bgColor;
static NSColor *fgColor;
const char *font;


static void draw_hook(void *arg, NSView *view)
{
	size_t i;
	struct screen *scr = arg;

	for (i = 0; i < scr->nr_hints; i++) {
		struct hint *h = &scr->hints[i];
		macos_draw_box(scr, bgColor,
				h->x, h->y, h->w, h->h, border_radius);

		macos_draw_text(scr, fgColor, font,
				h->x, h->y, h->w, h->h, h->label);
	}
}

void osx_hint_draw(struct screen *scr, struct hint *hints, size_t n)
{
	scr->nr_hints = n;
	memcpy(scr->hints, hints, sizeof(struct hint)*n);

	window_register_draw_hook(scr->overlay, draw_hook, scr);
}

void osx_init_hint(const char *bg, const char *fg, int _border_radius,
	       const char *font_family)
{
	bgColor = nscolor_from_hex(bg);
	fgColor = nscolor_from_hex(fg);

	border_radius = (float)_border_radius;
	font = font_family;
}

