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

#include "../../platform.h"
#include "impl.h"
#include <stdint.h>

static struct window *win;
static size_t nhints;
static struct hint hints[MAX_HINTS];
static uint8_t active_indices[MAX_HINTS];
static size_t box_height;
static float border_radius;

static NSDictionary *fontAttrs;
static NSColor *bgColor;
static NSColor *fgColor;

/* TODO: optimize. */

static void init_fontAttrs(const char *family, int h)
{
	int ptsz = h;
	CGSize size;
	do {
		NSFont *font = [NSFont fontWithName:[NSString stringWithUTF8String:family] size:ptsz];
		if (!font) {
			fprintf(stderr, "ERROR: %s is not a valid font\n", family);
			exit(-1);
		}
		fontAttrs = @{
			NSFontAttributeName: font,
			NSForegroundColorAttributeName: fgColor,
		};
		size = [@"m" sizeWithAttributes:fontAttrs];
		ptsz--;
	} while (size.height > h);
}

static void calculate_string_dimensions(const char *s, int *w, int *h)
{
	CGSize size = [[NSString stringWithUTF8String:s] sizeWithAttributes:fontAttrs];

	*w = size.width;
	*h = size.height;
}

static void draw_text_box(NSView *view, int x, int y, int w, int h, const char *s)
{
	int sw, sh;

	calculate_string_dimensions(s, &sw, &sh);

	NSString *str = [NSString stringWithUTF8String:s];

	[bgColor setFill];

	x = x - w/2;
	y = (int)view.frame.size.height-y-h/2;

	NSBezierPath *path = [NSBezierPath bezierPathWithRoundedRect:NSMakeRect((float)x, (float)y, (float)w, (float)h)
					    xRadius:border_radius
					    yRadius:border_radius];
	[path fill];

	[str
		drawAtPoint:NSMakePoint((float)x+(w-sw)/2, (float)y+(h-sh)/2)
		withAttributes:fontAttrs
	];
}

static void redraw(NSView *view)
{
	size_t i;
	int w = 0;
	int h = box_height;

	for (i = 0; i < nhints; i++) {
		int nw = [[NSString stringWithUTF8String:hints[i].label] sizeWithAttributes:fontAttrs].width;
		if (nw > w)
			w = nw;
	}

	w += 2;

	for (i = 0; i < nhints; i++) {
		if (active_indices[i])
			draw_text_box(view, hints[i].x, hints[i].y, w, h, hints[i].label);
	}
}

void init_hint(struct hint *_hints, size_t n, int _box_height, int _border_radius, 
			const char *bg, const char *fg, const char *font_family)
{
	size_t i;
	win = create_overlay_window(redraw);

	nhints = n;
	box_height = _box_height;
	bgColor = parse_color(bg);
	fgColor = parse_color(fg);
	border_radius = (float)_border_radius;

	init_fontAttrs(font_family, box_height);

	memcpy(hints, _hints, sizeof(struct hint)*n);
	for (i = 0; i < nhints; i++)
		active_indices[i] = 1;

	window_hide(win);
}

void hint_hide()
{
	window_hide(win);
}

void hint_show(uint8_t *indices)
{
	memcpy(active_indices, indices, sizeof(indices[0])*nhints);
	window_show(win);
	window_redraw(win);
}
