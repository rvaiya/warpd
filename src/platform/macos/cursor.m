/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include "macos.h"

static struct window *cw;
size_t sz = 0;

void cursor_draw(struct screen *scr, int x, int y)
{
	dispatch_sync(dispatch_get_main_queue(), ^{
		window_move(cw, scr, x+1, y-sz/2);
		window_show(cw);
	});
}

void cursor_hide() 
{
	dispatch_sync(dispatch_get_main_queue(), ^{
		window_hide(cw); 
	});
}

void init_cursor(const char *color, size_t _sz)
{
	sz = _sz;
	dispatch_sync(dispatch_get_main_queue(), ^{
		cw = create_window(color, sz, sz);
	});
}
