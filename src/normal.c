/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include "warpd.h"

static void redraw(screen_t scr, int x, int y, int force)
{
	screen_clear(scr);
	screen_draw_box(scr, x+1, y-cfg->cursor_size/2, cfg->cursor_size, cfg->cursor_size, cfg->cursor_color);

	platform_commit();
}

static void move(screen_t scr, int x, int y)
{
	mouse_move(scr, x, y);
	redraw(scr, x, y, 0);
}

struct input_event *normal_mode(struct input_event *start_ev)
{
	struct input_event *ev;
	screen_t	    scr;
	int		    sh, sw;
	int		    mx, my;

	input_grab_keyboard();

	mouse_get_position(&scr, &mx, &my);
	screen_get_dimensions(scr, &sw, &sh);

	mouse_hide();
	mouse_reset();

	while (1) {
		const int cursz = cfg->cursor_size;

		if (start_ev == NULL) {
			ev = input_next_event(10);
		} else {
			ev = start_ev;
			start_ev = NULL;
		}

		scroll_tick();
		if (mouse_process_key(ev, cfg->up, cfg->down, cfg->left, cfg->right)) {
			mouse_get_position(&scr, &mx, &my);
			redraw(scr, mx, my, 0);
			continue;
		}

		mouse_get_position(&scr, &mx, &my);

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
			move(scr, mx, 0);
		else if (input_event_eq(ev, cfg->bottom))
			move(scr, mx, sh - cursz);
		else if (input_event_eq(ev, cfg->middle))
			move(scr, mx, sh / 2);
		else if (input_event_eq(ev, cfg->start))
			move(scr, 0, my);
		else if (input_event_eq(ev, cfg->end))
			move(scr, sw - cursz, my);
		else if (input_event_eq(ev, cfg->hist_back)) {
			hist_add(mx, my);
			hist_prev();
			hist_get(&mx, &my);

			move(scr, mx, my);
		} else if (input_event_eq(ev, cfg->hist_forward)) {
			hist_next();
			hist_get(&mx, &my);

			move(scr, mx, my);
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

			for (i = 0; i < 3; i++) {
				const int btn = i + 1;
				int	  oneshot = 0;
				int	  match = 0;

				if (input_event_eq(ev,
						   cfg->oneshot_buttons[i])) {
					match = 1;
					oneshot = 1;
				} else if (input_event_eq(ev, cfg->buttons[i]))
					match = 1;

				if (match) {
					hist_add(mx, my);

					mouse_click(btn);

					if (oneshot) {
						const int timeout = cfg->oneshot_timeout;
						int timeleft = timeout;

						while (timeleft--) {
							struct input_event *ev = input_next_event(1);
							if (ev && ev->pressed &&
							    input_event_eq(ev,cfg->oneshot_buttons [i])) {
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
		mouse_get_position(&scr, &mx, &my);

		platform_commit();
	}

exit:
	hist_add(mx, my);

	mouse_show();
	screen_clear(scr);

	input_ungrab_keyboard();

	platform_commit();
	return ev;
}
