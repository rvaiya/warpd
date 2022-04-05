/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include "X.h"

struct grid {
	Window gridwins[32];
	size_t nc, nr;

	float width;
};

void grid_hide(struct grid *g)
{
	size_t i;

	for (i = 0; i < (g->nr + g->nc + 2); i++)
		XUnmapWindow(dpy, g->gridwins[i]);
}

static void redraw(struct grid *g, struct screen *scr, float ux, float uy,
		   float lx, float ly)
{
	size_t i;
	float  gap;
	float  offset;
	int    nw;

	nw = 0;
	offset = lx;
	gap = (ux - lx) / g->nc;

	for (i = 0; i < g->nc + 1; i++) {
		Window w = g->gridwins[nw++];

		XMoveResizeWindow(dpy, w, offset - (g->width / 2) + scr->x,
				  ly - g->width / 2 + scr->y, g->width,
				  uy - ly + g->width);

		XMapRaised(dpy, w);
		offset += gap;
	}

	offset = ly;
	gap = (uy - ly) / g->nr;

	for (i = 0; i < g->nr + 1; i++) {
		Window w = g->gridwins[nw++];

		XMoveResizeWindow(dpy, w, lx - (g->width / 2) + scr->x,
				  offset - (g->width / 2) + scr->y,
				  ux - lx + g->width, g->width);

		XMapRaised(dpy, w);
		offset += gap;
	}
}

struct grid *create_grid(const char *color, size_t width, size_t nc, size_t nr)
{
	size_t i;

	struct grid *g = malloc(sizeof(struct grid));

	g->nc = nc;
	g->nr = nr;
	g->width = (float)width;

	assert((g->nr + g->nc - 2) <
	       sizeof(g->gridwins) / sizeof(g->gridwins[0]));

	for (i = 0; i < (g->nr + g->nc + 2); i++)
		g->gridwins[i] = create_window(color);

	return g;
}

void grid_draw(struct grid *g, struct screen *scr, int _x, int _y, int _w,
	       int _h)
{
	float x = _x;
	float y = _y;
	float w = _w;
	float h = _h;

	redraw(g, scr, x + w - g->width / 2, y + h - g->width / 2,
	       x + g->width / 2, y + g->width / 2);
}
