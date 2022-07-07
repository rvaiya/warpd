/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include "warpd.h"

static void redraw(screen_t scr, int x, int y, int hide_cursor)
{
	int sw, sh;
	platform_screen_get_dimensions(scr, &sw, &sh);

	const int gap = 10;
	const int indicator_size = (config_get_int("indicator_size") * sh) / 1080;
	const char *indicator_color = config_get("indicator_color");
	const char *curcol = config_get("cursor_color");
	const char *indicator = config_get("indicator");
	const int cursz = config_get_int("cursor_size");

	platform_screen_clear(scr);

	if (!hide_cursor)
		platform_screen_draw_box(scr, x+1, y-cursz/2,
				cursz, cursz,
				curcol);


	if (!strcmp(indicator, "bottomleft"))
		platform_screen_draw_box(scr, gap, sh-indicator_size-gap, indicator_size, indicator_size, indicator_color);
	else if (!strcmp(indicator, "topleft"))
		platform_screen_draw_box(scr, gap, gap, indicator_size, indicator_size, indicator_color);
	else if (!strcmp(indicator, "topright"))
		platform_screen_draw_box(scr, sw-indicator_size-gap, gap, indicator_size, indicator_size, indicator_color);
	else if (!strcmp(indicator, "bottomright"))
		platform_screen_draw_box(scr, sw-indicator_size-gap, sh-indicator_size-gap, indicator_size, indicator_size, indicator_color);

	platform_commit();
}

static void move(screen_t scr, int x, int y)
{
	platform_mouse_move(scr, x, y);
	redraw(scr, x, y, 0);
}

struct input_event *normal_mode(struct input_event *start_ev, int oneshot)
{
	struct input_event *ev;
	screen_t scr;
	int sh, sw;
	int mx, my;

	platform_input_grab_keyboard();

	platform_mouse_get_position(&scr, &mx, &my);
	platform_screen_get_dimensions(scr, &sw, &sh);

	platform_mouse_hide();
	mouse_reset();
	redraw(scr, mx, my, 0);

	while (1) {
		const int cursz = config_get_int("cursor_size");

		if (start_ev == NULL) {
			ev = platform_input_next_event(10);
		} else {
			ev = start_ev;
			start_ev = NULL;
		}

		scroll_tick();
		if (mouse_process_key(ev, "up", "down", "left", "right")) {
			platform_mouse_get_position(&scr, &mx, &my);
			redraw(scr, mx, my, 0);
			continue;
		}

		if (!ev)
			continue;

		platform_mouse_get_position(&scr, &mx, &my);

		if (config_input_match(ev, "scroll_down", 1)) {
			redraw(scr, mx, my, 1);

			if (ev->pressed) {
				scroll_stop();
				scroll_accelerate(SCROLL_DOWN);
			} else
				scroll_decelerate();
		} else if (config_input_match(ev, "scroll_up", 1)) {
			redraw(scr, mx, my, 1);

			if (ev->pressed) {
				scroll_stop();
				scroll_accelerate(SCROLL_UP);
			} else
				scroll_decelerate();
		} else if (config_input_match(ev, "accelerator", 1)) {
			if (ev->pressed)
				mouse_fast();
			else
				mouse_normal();
		} else if (config_input_match(ev, "decelerator", 1)) {
			if (ev->pressed)
				mouse_slow();
			else
				mouse_normal();
		} else if (!ev->pressed) {
			goto next;
		}

		if (config_input_match(ev, "top", 1))
			move(scr, mx, cursz / 2);
		else if (config_input_match(ev, "bottom", 1))
			move(scr, mx, sh - cursz / 2);
		else if (config_input_match(ev, "middle", 1))
			move(scr, mx, sh / 2);
		else if (config_input_match(ev, "start", 1))
			move(scr, 1, my);
		else if (config_input_match(ev, "end", 1))
			move(scr, sw - cursz, my);
		else if (config_input_match(ev, "hist_back", 1)) {
			hist_add(mx, my);
			hist_prev();
			hist_get(&mx, &my);

			move(scr, mx, my);
		} else if (config_input_match(ev, "hist_forward", 1)) {
			hist_next();
			hist_get(&mx, &my);

			move(scr, mx, my);
		} else if (config_input_match(ev, "drag", 1)) {
			toggle_drag();
		} else if (config_input_match(ev, "copy_and_exit", 1)) {
			platform_copy_selection();
			ev = NULL;
			goto exit;
		} else if (config_input_match(ev, "exit", 1) ||
			   config_input_match(ev, "grid", 1) ||
			   config_input_match(ev, "screen", 1) ||
			   config_input_match(ev, "history", 1) ||
			   config_input_match(ev, "hint2", 1) ||
			   config_input_match(ev, "hint", 1)) {
			goto exit;
		} else if (config_input_match(ev, "print", 1)) {
			printf("%d %d\n", mx, my);
			fflush(stdout);
		} else { /* Mouse Buttons. */
			int btn;

			if ((btn = config_input_match(ev, "buttons", 0))) {
				if (oneshot)
					exit(btn);

				hist_add(mx, my);
				histfile_add(mx, my);
				platform_mouse_click(btn);
			} else if ((btn = config_input_match(ev, "oneshot_buttons", 0))) {
				hist_add(mx, my);
				platform_mouse_click(btn);

				const int timeout = config_get_int("oneshot_timeout");
				int timeleft = timeout;

				while (timeleft--) {
					struct input_event *ev = platform_input_next_event(1);
					if (ev && ev->pressed && 
						config_input_match(ev, "oneshot_buttons", 0)) {
						platform_mouse_click(btn);
						timeleft = timeout;
					}
				}

				exit(btn);
			}
		}
	next:
		platform_mouse_get_position(&scr, &mx, &my);

		platform_commit();
	}

exit:

	platform_mouse_show();
	platform_screen_clear(scr);

	platform_input_ungrab_keyboard();

	platform_commit();
	return ev;
}
