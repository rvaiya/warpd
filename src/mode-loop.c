#include "warpd.h"

int mode_loop(int initial_mode, int oneshot, int record_history)
{
	int mode = initial_mode;
	int rc = 0;
	struct input_event *ev = NULL;

	while (1) {
		int btn = 0;
		config_input_whitelist(NULL, 0);

		switch (mode) {
		case MODE_HISTORY:
			if (history_hint_mode() < 0)
				goto exit;

			ev = NULL;
			mode = MODE_NORMAL;
			break;
		case MODE_HINTSPEC:
			hintspec_mode();
			break;
		case MODE_NORMAL:
			ev = normal_mode(ev, oneshot);

			if (config_input_match(ev, "history"))
				mode = MODE_HISTORY;
			else if (config_input_match(ev, "hint"))
				mode = MODE_HINT;
			else if (config_input_match(ev, "hint2"))
				mode = MODE_HINT2;
			else if (config_input_match(ev, "grid"))
				mode = MODE_GRID;
			else if (config_input_match(ev, "screen"))
				mode = MODE_SCREEN_SELECTION;
			else if ((rc = config_input_match(ev, "oneshot_buttons")) || !ev) {
				goto exit;
			}
			else if (config_input_match(ev, "exit") || !ev) {
				rc = 0;
				goto exit;
			}

			break;
		case MODE_HINT2:
		case MODE_HINT:
			if (full_hint_mode(mode == MODE_HINT2) < 0)
				goto exit;

			ev = NULL;
			mode = MODE_NORMAL;
			break;
		case MODE_GRID:
			ev = grid_mode();
			if (config_input_match(ev, "grid_exit"))
				ev = NULL;
			mode = MODE_NORMAL;
			break;
		case MODE_SCREEN_SELECTION:
			screen_selection_mode();
			mode = MODE_NORMAL;
			ev = NULL;
			break;
		}

		if (oneshot && (initial_mode != MODE_NORMAL || (btn = config_input_match(ev, "buttons")))) {
			int x, y;
			screen_t scr;

			platform->mouse_get_position(&scr, NULL, NULL);
			platform->mouse_get_position(NULL, &x, &y);

			if (record_history)
				histfile_add(x, y);

			if (mode == MODE_HINTSPEC)
				printf("%d %d %s\n", x, y, last_selected_hint);
			else
				printf("%d %d\n", x, y);

			return btn;
		}
	}

exit:
	return rc;
}

