/*
 * warpd - A keyboard-driven modal pointer.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include <stddef.h>
#include <stdio.h>

#include "impl.h"

static struct window *win;

void cursor_show(int x, int y)
{
	window_move(win, x, y);
	window_show(win);
}

void cursor_hide() { window_hide(win); }

void init_cursor(const char *color, size_t sz)
{
	win = create_window(color, sz);
	window_hide(win);
	window_commit(win);
}

void cursor_commit() { window_commit(win); }
