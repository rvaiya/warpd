/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include "warpd.h"

static void redraw(screen_t scr, int x, int y, int hide_cursor)
{
	int sw, sh;
	screen_get_dimensions(scr, &sw, &sh);

	const int gap = 10;
	const int indicator_size = (config_get_int("indicator_size") * sh) / 1080;
	const char *indicator_color = config_get("indicator_color");
	const char *curcol = config_get("cursor_color");
	const char *indicator = config_get("indicator");
	const int cursz = config_get_int("cursor_size");

	screen_clear(scr);

	if (!hide_cursor)
		screen_draw_box(scr, x+1, y-cursz/2,
				cursz, cursz,
				curcol);


	if (!strcmp(indicator, "bottomleft"))
		screen_draw_box(scr, gap, sh-indicator_size-gap, indicator_size, indicator_size, indicator_color);
	else if (!strcmp(indicator, "topleft"))
		screen_draw_box(scr, gap, gap, indicator_size, indicator_size, indicator_color);
	else if (!strcmp(indicator, "topright"))
		screen_draw_box(scr, sw-indicator_size-gap, gap, indicator_size, indicator_size, indicator_color);
	else if (!strcmp(indicator, "bottomright"))
		screen_draw_box(scr, sw-indicator_size-gap, sh-indicator_size-gap, indicator_size, indicator_size, indicator_color);

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
	screen_t scr;
	int sh, sw;
	int mx, my;

	input_grab_keyboard();

	mouse_get_position(&scr, &mx, &my);
	screen_get_dimensions(scr, &sw, &sh);

	mouse_hide();
	mouse_reset();
	redraw(scr, mx, my, 0);

	while (1) {
		const int cursz = config_get_int("cursor_size");

		if (start_ev == NULL) {
			ev = input_next_event(10);
		} else {
			ev = start_ev;
			start_ev = NULL;
		}

		scroll_tick();
		if (mouse_process_key(ev, "up", "down", "left", "right")) {
			mouse_get_position(&scr, &mx, &my);
			redraw(scr, mx, my, 0);
			continue;
		}

		if (!ev)
			continue;

		mouse_get_position(&scr, &mx, &my);

		if (config_input_match(ev, "scroll_down")) {
			redraw(scr, mx, my, 1);

			if (ev->pressed) {
				scroll_stop();
				scroll_accelerate(SCROLL_DOWN);
			} else
				scroll_decelerate();
		} else if (config_input_match(ev, "scroll_up")) {
			redraw(scr, mx, my, 1);

			if (ev->pressed) {
				scroll_stop();
				scroll_accelerate(SCROLL_UP);
			} else
				scroll_decelerate();
		} else if (config_input_match(ev, "accelerator")) {
			if (ev->pressed)
				mouse_fast();
			else
				mouse_slow();
		} else if (!ev->pressed) {
			goto next;
		}

		if (config_input_match(ev, "top"))
			move(scr, mx, cursz / 2);
		else if (config_input_match(ev, "bottom"))
			move(scr, mx, sh - cursz / 2);
		else if (config_input_match(ev, "middle"))
			move(scr, mx, sh / 2);
		else if (config_input_match(ev, "start"))
			move(scr, 1, my);
		else if (config_input_match(ev, "end"))
			move(scr, sw - cursz, my);
		else if (config_input_match(ev, "hist_back")) {
			hist_add(mx, my);
			hist_prev();
			hist_get(&mx, &my);

			move(scr, mx, my);
		} else if (config_input_match(ev, "hist_forward")) {
			hist_next();
			hist_get(&mx, &my);

			move(scr, mx, my);
		} else if (config_input_match(ev, "drag")) {
			toggle_drag();
		} else if (config_input_match(ev, "copy_and_exit")) {
			copy_selection();
			ev = NULL;
			goto exit;
		} else if (config_input_match(ev, "exit") ||
			   config_input_match(ev, "grid") ||
			   config_input_match(ev, "screen") ||
			   config_input_match(ev, "hint")) {
			goto exit;
		} else { /* Mouse Buttons. */
			int btn;

			if ((btn = config_input_match(ev, "buttons"))) {
				hist_add(mx, my);
				mouse_click(btn);
			} else if ((btn = config_input_match(ev, "oneshot_buttons"))) {
				hist_add(mx, my);
				mouse_click(btn);

				const int timeout = config_get_int("oneshot_timeout");
				int timeleft = timeout;

				while (timeleft--) {
					struct input_event *ev = input_next_event(1);
					if (ev && ev->pressed && 
						config_input_match(ev, "oneshot_buttons")) {
						mouse_click(btn);
						timeleft = timeout;
					}
				}

				ev = NULL;
				goto exit;
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
