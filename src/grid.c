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

/* returns the terminating input event. */
struct input_event *grid_mode()
{
	struct input_event *ev;
	int gx, gy, gw, gh;

	const int nc = cfg->grid_nc;
	const int nr = cfg->grid_nr;

	/* grid boundaries */

	gx = 0;
	gy = 0;
	screen_get_dimensions(&gw, &gh);

	input_grab_keyboard();
	mouse_hide();
	mouse_reset();
	mouse_move(gw/2, gh/2);

	while(1) {
		ev = input_next_event(10);
		int cx, cy;

		size_t i;
		size_t idx = 0;

		mouse_process_key(ev,
			cfg->grid_up,
			cfg->grid_down,
			cfg->grid_left,
			cfg->grid_right);

		if (!ev || !ev->pressed)
			goto next;

		for (idx = 0; idx < cfg->grid_keys_sz; idx++)
			if (input_event_eq(ev, cfg->grid_keys[idx])) {
				if ((int)idx >= nc*nr)
					continue;

				gy += (gh/nr)*(idx/nc);
				gx += (gw/nc)*(idx%nc);

				gh /= nr;
				gw /= nc;

				gh = gh ? gh : 1;
				gw = gw ? gw : 1;

				mouse_move(gx + (gw / 2), gy + (gh / 2));
			}

		for (i = 0; i < cfg->buttons_sz; i++) {
			if (input_event_eq(ev, cfg->buttons[i]))
				goto exit;
		}

		for (i = 0; i < cfg->oneshot_buttons_sz; i++) {
			if (input_event_eq(ev, cfg->oneshot_buttons[i]))
				goto exit;
		}

		if (input_event_eq(ev, cfg->drag))
			goto exit;

		if (input_event_eq(ev, cfg->exit))
			goto exit;

next:
		mouse_get_position(&cx, &cy);
		gx = cx - gw/2;
		gy = cy - gh/2;

		cursor_show(cx - cfg->cursor_size/2, cy - cfg->cursor_size/2);

		grid_draw(gx, gy, gw, gh);
	}

exit:
	grid_hide();
	mouse_show();

	input_ungrab_keyboard();
	return ev;
}
