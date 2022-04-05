#include "macos.h"

static struct hint	*hints;
static size_t		 nr_hints;

static struct screen	*scr;

static float		border_radius;

static NSColor		*bgColor;
static NSColor		*fgColor;
const char		*font;


static void draw_hook(void *arg, NSView *view)
{
	size_t i;

	if (!hints)
		return;

	for (i = 0; i < nr_hints; i++) {
		struct hint *h = &hints[i];
		macos_draw_box(scr, bgColor,
				h->x, h->y, h->w, h->h, border_radius);

		macos_draw_text(scr, fgColor, font,
				h->x, h->y, h->w, h->h, h->label);
	}
}

void hint_hide()
{
	size_t i;

	for (i = 0; i < nr_screens; i++) {
		struct screen *scr = &screens[i];
		window_unregister_draw_fn(scr->overlay, draw_hook, NULL);
	}

	dispatch_sync(dispatch_get_main_queue(), ^{
		window_hide(scr->overlay); 
	});
}

void hint_draw(struct screen *_scr, struct hint *_hints, size_t n)
{
	hints = _hints;
	nr_hints = n;
	scr = _scr;
	window_register_draw_fn(scr->overlay, draw_hook, NULL);

	dispatch_sync(dispatch_get_main_queue(), ^{
		window_show(scr->overlay); 
	});
}

void init_hint(const char *bg, const char *fg, int _border_radius,
	       const char *font_family)
{
	bgColor = nscolor_from_hex(bg);
	fgColor = nscolor_from_hex(fg);

	border_radius = (float)_border_radius;
	font = font_family;
}

