/*
 * warpd - A keyboard-driven modal pointer.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#ifndef IMPL_H
#define IMPL_H

#include "../../platform.h"
#include <X11/Xlib.h>

extern Display *dpy;

struct pixmap {
	GC     gc;
	Pixmap map;
	int    w;
	int    h;
};

Window create_window(const char *color, int x, int y, int w, int h);

void mouse_move_abs(int x, int y);
void mouse_hide();
void mouse_unhide();
void mouse_get_current_position(int *x, int *y);

void init_mouse();

int hex_to_rgba(const char *str, uint8_t *r, uint8_t *g, uint8_t *b,
		uint8_t *a);

struct pixmap *create_pixmap(const char *color, int w, int h);
void	       pixmap_copy(struct pixmap *pixmap, Window win);
#endif
