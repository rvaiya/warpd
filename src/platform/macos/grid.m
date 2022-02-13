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

#include <Cocoa/Cocoa.h>
#include "impl.h"

static int nr;
static int nc;
static int gsz;
static NSColor *borderColor = NULL;
static int gx, gy, gw, gh;
static struct window *win;

static void draw_box(int x, int y, int w, int h)
{
	int sw, sh;
	NSScreen *scr;

	[borderColor setFill];

	scr = [NSScreen mainScreen];
	sw = [scr frame].size.width;
	sh = [scr frame].size.height;

	CGContextFillRect([NSGraphicsContext currentContext].CGContext, CGRectMake(x, sh-y-h, w, h));
}

static void redraw(NSView *view)
{
	int i;
	int y, x;
	int rowgap, colgap;

	y = gy;

	rowgap = (gh-gsz+1)/nr;
	colgap = (gw-gsz+1)/nc;

	for (i = 0; i < nr+1; i++) {
		draw_box(gx,y,gw,gsz);
		y += rowgap;
	}

	x = gx;
	for (i = 0; i < nc+1; i++) {
		draw_box(x,gy,gsz,gh);
		x += colgap;
	}
}


void init_grid(const char *_color, size_t _thickness, size_t _nc, size_t _nr)
{
	nc = _nc;
	nr = _nr;
	borderColor = parse_color(_color);
	gsz = _thickness;
	win = create_overlay_window(redraw);
}

void grid_hide()
{
	cursor_hide();
	window_hide(win);
}

void grid_draw(int x, int y, int w, int h)
{
	gx = x;
	gy = y;
	gw = w;
	gh = h;

	window_redraw(win);
	window_show(win);
}
