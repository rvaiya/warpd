/* Copyright © 2019 Raheman Vaiya.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef IMPL_H
#define IMPL_H

#include "../../platform.h"
#include <X11/Xlib.h>

extern Display *dpy;

struct pixmap {
	GC gc;
	Pixmap pixmap;
	int w;
	int h;
};

Window	create_window(const char *color, int x, int y, int w, int h);

void	window_commit();
void	window_show(Window win);
void	window_hide(Window win);
void	window_move(Window win, int x, int y);
void	window_resize(Window win, int w, int h);

void	mouse_move_abs(int x, int y);
void	mouse_hide();
void	mouse_unhide();
void	mouse_get_current_position(int *x, int *y);

void	init_mouse();

int	hex_to_rgba(const char *str, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);

struct pixmap	*create_pixmap(const char *color, int w, int h);
void 		 pixmap_copy(struct pixmap *pixmap, Window win);
#endif
