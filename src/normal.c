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

struct input_event *normal_mode(struct input_event *start_ev)
{
	struct input_event *ev;
	int sh, sw;
	int cy, cx;

	input_grab_keyboard();

	screen_get_dimensions(&sw, &sh);
	mouse_get_position(&cx, &cy);
	mouse_hide();
	mouse_reset();

	while(1) {
		const int cursz = cfg->cursor_size;

		if (start_ev == NULL) {
			ev = input_next_event(10);
		} else {
			ev = start_ev;
			start_ev = NULL;
		}

		if (mouse_process_key(ev, cfg->up, cfg->down, cfg->left, cfg->right))
			goto next;
		if (!ev)
			goto next;

		mouse_get_position(&cx, &cy);

		if (input_event_eq(ev, cfg->scroll_down)) {
			if (ev->pressed) {
				scroll_stop();
				scroll_accelerate(SCROLL_DOWN);
			} else
				scroll_decelerate();
		} else if (input_event_eq(ev, cfg->scroll_up)) {
			if (ev->pressed) {
				scroll_stop();
				scroll_accelerate(SCROLL_UP);
			} else
				scroll_decelerate();
		} else if (!ev->pressed) {
			goto next;
		}

		if (input_event_eq(ev, cfg->top))
			mouse_move(cx, 0);
		else if (input_event_eq(ev, cfg->bottom))
			mouse_move(cx, sh - cursz);
		else if (input_event_eq(ev, cfg->middle))
			mouse_move(cx, sh/2);
		else if (input_event_eq(ev, cfg->start))
			mouse_move(0, cy);
		else if (input_event_eq(ev, cfg->end))
			mouse_move(sw - cursz, cy);
		else if (input_event_eq(ev, cfg->hist_back)) {
			hist_add(cx, cy);
			hist_prev();
			hist_get(&cx, &cy);

			mouse_move(cx, cy);
		} else if (input_event_eq(ev, cfg->hist_forward)) {
			hist_next();
			hist_get(&cx, &cy);

			mouse_move(cx, cy);
		} else if (input_event_eq(ev, cfg->drag)) {
			toggle_drag();
		} else if (input_event_eq(ev, cfg->copy_and_exit)) {
			copy_selection();
			ev = NULL;
			goto exit;
		} else if (input_event_eq(ev, cfg->exit) ||
			   input_event_eq(ev, cfg->grid) ||
			   input_event_eq(ev, cfg->hint)) {
			goto exit;
		} else { /* Mouse Buttons. */
			size_t i;

			for(i = 0;i < 3;i++) {
				const int btn = i+1;
				int oneshot = 0;
				int match = 0;

				if (input_event_eq(ev, cfg->oneshot_buttons[i])) {
					match = 1;
					oneshot = 1;
				} else if (input_event_eq(ev, cfg->buttons[i]))
					match = 1;

				if (match) {
					hist_add(cx, cy);

					mouse_click(btn);

					if (oneshot) {
						const int timeout = cfg->oneshot_timeout;
						int timeleft = timeout;

						while (timeleft--) {
							struct input_event *ev = input_next_event(1);
							if (ev && ev->pressed && input_event_eq(ev, cfg->oneshot_buttons[i])) {
								mouse_click(btn);
								timeleft = timeout;
							}
						}

						ev = NULL;
						goto exit;
					}
				}
			}
		}
next:
		mouse_get_position(&cx, &cy);
		cursor_show(cx+1, cy+1);
		scroll_tick();
	}

exit:
	hist_add(cx, cy);

	mouse_show();
	cursor_hide();

	input_ungrab_keyboard();
	return ev;
}

void init_normal_mode()
{
	init_cursor(cfg->cursor_color, cfg->cursor_size);
	cursor_hide();
}
