#include "warpd.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

struct config_entry *config = NULL;

static struct {
	const char *key;
	const char *val;

	const char *description;
} options[] = {
	{ "hint_activation_key", "A-M-x", "Activates hint mode." },
	{ "grid_activation_key", "A-M-g", "Activates grid mode and allows for further manipulation of the pointer using the mapped keys." },
	{ "screen_activation_key", "A-M-s", "Activate (s)creen selection mode." },
	{ "activation_key", "A-M-c", "Activate normal movement mode (manual (c)ursor movement)." },
	{ "hint_oneshot_key", "A-M-l", "Activate hint mode and exit upon selection." },
	{ "repeat_interval", "20", "The number of milliseconds before repeating a movement event." },
	{ "speed", "220", "Pointer speed in pixels/second." },
	{ "max_speed", "1600", "The maximum pointer speed." },
	{ "decelerator_speed", "50", "Pointer speed while decelerator is depressed." },
	{ "acceleration", "700", "Pointer acceleration in pixels/second^2." },
	{ "accelerator_acceleration", "2900", "Pointer acceleration while the accelerator is depressed." },
	{ "accelerator", "a", "Increase the acceleration of the pointer while held." },
	{ "decelerator", "d", "Decrease the speed of the pointer while held." },
	{ "buttons", "m , .",  "A space separated list of mouse buttons (2 is middle click)." },
	{ "oneshot_buttons", "n - /", "Oneshot mouse buttons (deactivate on click)." },
	{ "oneshot_timeout", "300", "The length of time in milliseconds to wait for a second click after a oneshot key has been pressed." },
	{ "grid_exit", "c", "Exit grid mode and return to normal mode." },
	{ "hint_exit", "esc", "The exit key used for hint mode." },
	{ "exit", "esc", "Exit the currently active warpd session." },
	{ "drag", "v", "Toggle drag mode (mneominc (v)isual mode)." },
	{ "copy_and_exit", "c", "Send the copy key and exit (useful in combination with v)." },

	{ "history", ";", "Activate hint history mode while in normal mode." },
	{ "hint", "x", "Activate hint mode while in normal mode (mnemonic: x marks the spot?)." },
	{ "grid", "g", "Activate (g)rid mode while in normal mode." },
	{ "screen", "s", "Activate (s)creen selection while in normal mode." },

	{ "left", "h", "Move the cursor left in normal mode." },
	{ "down", "j", "Move the cursor down in normal mode." },
	{ "up", "k", "Move the cursor up in normal mode." },
	{ "right", "l", "Move the cursor right in normal mode." },
	{ "cursor_color", "#FF4500", "The color of the pointer in normal mode (rgba hex value)." },
	{ "cursor_size", "7", "The height of the pointer in normal mode." },
	{ "top", "H", "Moves the cursor to the top of the screen in normal mode." },
	{ "middle", "M", "Moves the cursor to the middle of the screen in normal mode." },
	{ "bottom", "L", "Moves the cursor to the bottom of the screen in normal mode." },
	{ "start", "0", "Moves the cursor to the leftmost corner of the screen in normal mode." },
	{ "end", "$", "Moves the cursor to the rightmost corner of the screen in normal mode." },

	{ "hist_back", "C-o", "Move to the last position in the history stack." },
	{ "hist_forward", "C-i", "Move to the next position in the history stack." },
	{ "hist_hint_size", "2", "History hint size as a percentage of screen height." },

	{ "grid_nr", "2", "The number of rows in the grid." },
	{ "grid_nc", "2", "The number of columns in the grid." },
	{ "grid_up", "w", "Move the grid up." },
	{ "grid_left", "a", "Move the grid left." },
	{ "grid_down", "s", "Move the grid down." },
	{ "grid_right", "d", "Move the grid right." },
	{ "grid_keys", "u i j k", "A sequence of comma delimited keybindings which are ordered bookwise with respect to grid position." },
	{ "grid_color", "#1c1c1e", "The color of the grid." },
	{ "grid_size", "4", "The thickness of grid lines in pixels." },
	{ "grid_border_size", "0", "The thickness of the grid border in pixels." },
	{ "grid_border_color", "#ffffff", "The color of the grid border." },

	{ "hint_size", "71", "Hint size (range: 1-100)" },
	{ "hint_bgcolor", "#1c1c1e", "The background hint color." },
	{ "hint_fgcolor", "#a1aba7", "The foreground hint color." },
	{ "hint_border_radius", "3", "Border radius." },
	{ "hint_chars", "abcdefghijklmnopqrstuvwxyz", "The character set from which hints are generated. The total number of hints is the square of the size of this string. It may be desirable to increase this for larger screens or trim it to increase gaps between hints." },
	{ "hint_font", "Arial", "The font name used by hints. Note: This is platform specific, in X it corresponds to a valid xft font name, on macos it corresponds to a postscript name." },

	{ "scroll_down", "e", "Scroll down key." },
	{ "scroll_up", "r", "Scroll up key." },
	{ "screen_chars", "jkl;asdfg", "The characters used for screen selection." },
	{ "scroll_speed", "300", "Initial scroll speed in units/second (unit varies by platform)." },
	{ "scroll_max_speed", "9000", "Maximum scroll speed." },
	{ "scroll_acceleration", "1600", "Scroll acceleration in units/second^2." },
	{ "scroll_deceleration", "-3400", "Scroll deceleration." },

	{ "indicator", "none", "Specifies an optional visual indicator to be displayed while normal mode is active, must be one of: topright, topleft, bottomright, bottomleft, none" },
	{ "indicator_color", "#00ff00", "The color of the visual indicator color." },
	{ "indicator_size", "12", "The size of the visual indicator in pixels." },
};

const char *config_get(const char *key)
{
	struct config_entry *ent;

	for (ent = config; ent; ent = ent->next)
		if (!strcmp(ent->key, key))
			return ent->value;

	fprintf(stderr, "FATAL: unrecognized config entry: %s\n", key);
	exit(-1);
}

int config_get_int(const char *key)
{
	return atoi(config_get(key));
}

static void config_add(const char *key, const char *val)
{
	struct config_entry *ent;
	ent = malloc(sizeof(struct config_entry));

	ent->key = key;
	ent->value = val;
	ent->next = config;

	config = ent;
}

void parse_config(const char *path)
{
	size_t i;

	FILE *fh = fopen(path, "r");
	if (fh) {
		while (1) {
			int ret;
			size_t sz = 0;
			char *line = NULL;
			char *delim;

			if ((ret=getline(&line, &sz, fh)) < 0) {
				free(line);
				break;
			}

			line[ret-1] = 0;
			delim = strchr(line, ':');

			if (!delim) {
				free(line);
				continue;
			}

			*delim = 0;
			while (*++delim == ' ');

			config_add(line, delim);
		}
		fclose(fh);
	}

	for (i = 0; i < sizeof(options) / sizeof(options[0]); i++) {
		struct config_entry *ent;

		for (ent = config; ent; ent = ent->next) {
			if (!strcmp(ent->key, options[i].key))
				break;
		}

		if (!ent)
			config_add(options[i].key, options[i].val);
	}
}

/*
 * Consumes an input event and the name of a config option corresponding
 * to a set of keys and returns the 1-based index of the matching key
 * (if any).
 */

int config_input_match(struct input_event *ev, const char *config_key, int strict)
{
	char *tok;
	struct config_entry *ent;

	for (ent = config; ent; ent = ent->next) {
		int i = 1;
		if (!strcmp(config_key, ent->key)) {
			char buf[1024];
			snprintf(buf, sizeof buf, "%s", ent->value);

			for (tok = strtok(buf, " "); tok; tok = strtok(NULL, " ")) {
				if (input_eq(ev, tok, strict))
					return i;
				i++;
			}

		}
	}


	return 0;
}

void config_print_options()
{
	size_t i;
	for (i = 0; i < sizeof(options)/sizeof(options[0]); i++)
		printf("%s: %s (default: %s)\n", options[i].key, options[i].description, options[i].val);
}
