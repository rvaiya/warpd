/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include "macos.h"

struct grid {
	NSColor	*color;
	size_t	nc;
	size_t	nr;
	float	sz;

	float	w;
	float	h;
	float	x;
	float	y;

	const char *c;
	struct screen	*scr;
};

static void draw_hook(void *arg, NSView *view)
{
	const struct grid *g = arg;

	size_t	i;
	float	gap;
	float	offset;

	float	ux = g->x + g->w;
	float	lx = g->x;
	float	uy = g->y + g->h;
	float	ly = g->y;

	offset = lx;
	gap = (ux - lx) / g->nc;

	for (i = 0; i < g->nc + 1; i++) {
		macos_draw_box(g->scr, g->color, offset - (g->sz / 2), ly - g->sz / 2, g->sz,
			 uy - ly + g->sz, 0);
		offset += gap;
	}

	offset = ly;
	gap = (uy - ly) / g->nr;

	for (i = 0; i < g->nr + 1; i++) {
		macos_draw_box(g->scr, g->color, lx - (g->sz / 2), offset - (g->sz / 2),
			 ux - lx + g->sz, g->sz, 0);
		offset += gap;
	}
}

struct grid *create_grid(const char *color,
			 size_t width, size_t nc, size_t nr)
{
	struct grid *g = calloc(1, sizeof(struct grid));

	g->nc = nc;
	g->nr = nr;
	g->sz = width;

	g->color = nscolor_from_hex(color);
	g->scr = NULL;
	g->c = color;

	return g;
}

void grid_hide(struct grid *g)
{
	if (!g->scr)
		return;

	window_unregister_draw_fn(g->scr->overlay, draw_hook, g);
	dispatch_sync(dispatch_get_main_queue(), ^{
		window_hide(g->scr->overlay);
	});
	g->scr = NULL;
}

void grid_draw(struct grid *g, struct screen *scr, int x, int y, int w, int h)
{
	g->x = (float)x + g->sz / 2;
	g->y = (float)y + g->sz / 2;
	g->w = (float)w - g->sz;
	g->h = (float)h - g->sz;

	if (g->scr)
		window_unregister_draw_fn(g->scr->overlay, draw_hook, g);

	g->scr = scr;

	window_register_draw_fn(scr->overlay, draw_hook, g);
	dispatch_sync(dispatch_get_main_queue(), ^{
		window_show(scr->overlay); 
	});
}
