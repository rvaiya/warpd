#include "warpd.h"

void screen_selection_mode()
{
	size_t i;
	size_t n;
	screen_t screens[MAX_SCREENS];
	struct input_event *ev;

	screen_list(screens, &n);
	assert(strlen(cfg->screen_chars) >= n);

	for (i = 0; i < n; i++) {
		struct hint hint;
		int w, h;
		screen_get_dimensions(screens[i], &w, &h);

		hint.x = w / 2 - 25;
		hint.y = h / 2 - 25;
		hint.w = 50;
		hint.h = 50;

		hint.label[0] = cfg->screen_chars[i];
		hint.label[1] = 0;

		hint_draw(screens[i], &hint, 1);
	}

	platform_commit();

	input_grab_keyboard();
	while (1) {
		ev = input_next_event(0);
		if (ev->pressed)
			break;
	}
	input_ungrab_keyboard();

	for (i = 0; i < n; i++) {
		char s[2] = {cfg->screen_chars[i], 0};

		if (input_event_eq(ev, s)) {
			int w, h;
			screen_get_dimensions(screens[i], &w, &h);
			mouse_move(screens[i], w/2, h/2);
		}
	}

	for (i = 0; i < n; i++)
		screen_clear(screens[i]);

	platform_commit();
}
