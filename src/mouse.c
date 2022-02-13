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

#include "warpd.h"

static int left = 0;
static int right = 0;
static int up = 0;
static int down = 0;

static long get_time_ms()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (ts.tv_nsec/1E6) + (ts.tv_sec * 1E3);
}

static int tonum(uint8_t code)
{
	char c = input_lookup_name(code)[0];

	if (c > '9' || c < '0')
		return -1;

	return c-'0';
}

void mouse_reset()
{
	left = 0;
	right = 0;
	up = 0;
	down = 0;
}

/* returns 1 if handled */
int mouse_process_key(struct input_event *ev, 
			const char *up_key, const char *down_key,
			const char *left_key, const char *right_key)
{
	static int ocx = 0;
	static int ocy = 0;
	static long processed_pixels = 0;
	static int opnum = 0;

	int handled = 0;
	int n;

	int dx, dy;
	int sh, sw;
	int cx, cy;
	int old_pp;
	float pixels_per_ms;

	mouse_get_position(&cx, &cy);
	screen_get_dimensions(&sw, &sh);

	pixels_per_ms = (((float)sh / 1000 / 100) * cfg->speed);

	if (!ev)
		goto exit;

	if ((n=tonum(ev->code)) != -1 && ev->mods == 0) {
		if (ev->pressed)
			opnum = opnum*10 + n;

		/* Allow 0 on its own as a special case. */
		if (opnum != 0)
			handled = 1;

		goto exit;
	}

	if (input_event_eq(ev, down_key)) {
		down = ev->pressed;
		handled++;
	} else if (input_event_eq(ev, left_key)) {
		left = ev->pressed;
		handled++;
	} else if (input_event_eq(ev, right_key)) {
		right = ev->pressed;
		handled++;
	} else if (input_event_eq(ev, up_key)) {
		up = ev->pressed;
		handled++;
	}

	if (handled && opnum) {
		const int inc = sh / 100;
		const int x = right - left;
		const int y = down - up;

		cx += inc*opnum*x;
		cy += inc*opnum*y;

		opnum = 0;

		left = 0;
		right = 0;
		up = 0;
		down = 0;
	}

exit:
	/* update the cursor position. */
	dx = right - left;
	dy = down - up;

	old_pp = processed_pixels;
	processed_pixels = get_time_ms() * pixels_per_ms;

	cx += dx * (processed_pixels - old_pp);
	cy += dy * (processed_pixels - old_pp);

	if (cx < 0)
		cx = 0;
	if (cy < 0)
		cy = 0;
	if (cy > sh)
		cy = sh;
	if (cx > sw)
		cx = sw;

	if (ocx != cx || ocy != cy)
		mouse_move(cx, cy);

	ocx = cx;
	ocy = cy;

	return handled;
}
