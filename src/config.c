#include "warpd.h"
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

struct config_entry *config = NULL;

static struct {
	const char *key;
	const char *val;

	const char *description;
	enum option_type type;
} options[] = {
	{ "hint_activation_key", "A-M-x", "Activates hint mode.", OPT_ACTIVATION_KEY },
	{ "hint2_activation_key", "A-M-X", "Activate two pass hint mode.", OPT_ACTIVATION_KEY },
	{ "grid_activation_key", "A-M-g", "Activates grid mode and allows for further manipulation of the pointer using the mapped keys.", OPT_ACTIVATION_KEY },
	{ "history_activation_key", "A-M-h", "Activate history mode.", OPT_ACTIVATION_KEY },
	{ "screen_activation_key", "A-M-s", "Activate (s)creen selection mode.", OPT_ACTIVATION_KEY },
	{ "activation_key", "A-M-c", "Activate normal movement mode (manual (c)ursor movement).", OPT_ACTIVATION_KEY },

	{ "hint_oneshot_key", "A-M-l", "Activate hint mode and exit upon selection.", OPT_ACTIVATION_KEY },
	{ "hint2_oneshot_key", "A-M-L", "Activate two pass hint mode and exit upon selection.", OPT_ACTIVATION_KEY },

	/* Normal mode keys */

	{ "exit", "esc", "Exit the currently active warpd session.", OPT_NORMAL_KEY },
	{ "drag", "v", "Toggle drag mode (mnemonic (v)isual mode).", OPT_NORMAL_KEY },
	{ "copy_and_exit", "c", "Send the copy key and exit (useful in combination with v).", OPT_NORMAL_KEY },
	{ "accelerator", "a", "Increase the acceleration of the pointer while held.", OPT_NORMAL_KEY },
	{ "decelerator", "d", "Decrease the speed of the pointer while held.", OPT_NORMAL_KEY },
	{ "buttons", "m , .",  "A space separated list of mouse buttons (2 is middle click).", OPT_NORMAL_KEY },
	{ "oneshot_buttons", "n - /", "Oneshot mouse buttons (deactivate on click).", OPT_NORMAL_KEY },

	{ "print", "p", "Print the current mouse coordinates to stdout (useful for scripts).", OPT_NORMAL_KEY },
	{ "history", ";", "Activate hint history mode while in normal mode.", OPT_NORMAL_KEY },
	{ "hint", "x", "Activate hint mode while in normal mode (mnemonic: x marks the spot?).", OPT_NORMAL_KEY },
	{ "hint2", "X", "Activate two pass hint mode.", OPT_NORMAL_KEY },
	{ "grid", "g", "Activate (g)rid mode while in normal mode.", OPT_NORMAL_KEY },
	{ "screen", "s", "Activate (s)creen selection while in normal mode.", OPT_NORMAL_KEY },

	{ "left", "h", "Move the cursor left in normal mode.", OPT_NORMAL_KEY },
	{ "down", "j", "Move the cursor down in normal mode.", OPT_NORMAL_KEY },
	{ "up", "k", "Move the cursor up in normal mode.", OPT_NORMAL_KEY },
	{ "right", "l", "Move the cursor right in normal mode.", OPT_NORMAL_KEY },
	{ "top", "H", "Moves the cursor to the top of the screen in normal mode.", OPT_NORMAL_KEY },
	{ "middle", "M", "Moves the cursor to the middle of the screen in normal mode.", OPT_NORMAL_KEY },
	{ "bottom", "L", "Moves the cursor to the bottom of the screen in normal mode.", OPT_NORMAL_KEY },
	{ "start", "0", "Moves the cursor to the leftmost corner of the screen in normal mode.", OPT_NORMAL_KEY },
	{ "end", "$", "Moves the cursor to the rightmost corner of the screen in normal mode.", OPT_NORMAL_KEY },

	{ "scroll_down", "e", "Scroll down key.", OPT_NORMAL_KEY },
	{ "scroll_up", "r", "Scroll up key.", OPT_NORMAL_KEY },

	{ "cursor_color", "#FF4500", "The color of the pointer in normal mode (rgba hex value).", OPT_STRING },

	{ "cursor_size", "7", "The height of the pointer in normal mode.", OPT_INT },
	{ "repeat_interval", "20", "The number of milliseconds before repeating a movement event.", OPT_INT },
	{ "speed", "220", "Pointer speed in pixels/second.", OPT_INT },
	{ "max_speed", "1600", "The maximum pointer speed.", OPT_INT },
	{ "decelerator_speed", "50", "Pointer speed while decelerator is depressed.", OPT_INT },
	{ "acceleration", "700", "Pointer acceleration in pixels/second^2.", OPT_INT },
	{ "accelerator_acceleration", "2900", "Pointer acceleration while the accelerator is depressed.", OPT_INT },
	{ "oneshot_timeout", "300", "The length of time in milliseconds to wait for a second click after a oneshot key has been pressed.", OPT_INT },
	{ "hist_hint_size", "2", "History hint size as a percentage of screen height.", OPT_INT },
	{ "grid_nr", "2", "The number of rows in the grid.", OPT_INT },
	{ "grid_nc", "2", "The number of columns in the grid.", OPT_INT },

	{ "hist_back", "C-o", "Move to the last position in the history stack.", OPT_NORMAL_KEY },
	{ "hist_forward", "C-i", "Move to the next position in the history stack.", OPT_NORMAL_KEY },

	{ "grid_up", "w", "Move the grid up.", OPT_GRID_KEY },
	{ "grid_left", "a", "Move the grid left.", OPT_GRID_KEY },
	{ "grid_down", "s", "Move the grid down.", OPT_GRID_KEY },
	{ "grid_right", "d", "Move the grid right.", OPT_GRID_KEY },
	{ "grid_keys", "u i j k", "A sequence of comma delimited keybindings which are ordered bookwise with respect to grid position.", OPT_GRID_KEY },
	{ "grid_exit", "c", "Exit grid mode and return to normal mode.", OPT_GRID_KEY },

	{ "grid_size", "4", "The thickness of grid lines in pixels.", OPT_INT },
	{ "grid_border_size", "0", "The thickness of the grid border in pixels.", OPT_INT },

	{ "grid_color", "#1c1c1e", "The color of the grid.", OPT_STRING },
	{ "grid_border_color", "#ffffff", "The color of the grid border.", OPT_STRING },

	{ "hint_bgcolor", "#1c1c1e", "The background hint color.", OPT_STRING },
	{ "hint_fgcolor", "#a1aba7", "The foreground hint color.", OPT_STRING },
	{ "hint_chars", "abcdefghijklmnopqrstuvwxyz", "The character set from which hints are generated. The total number of hints is the square of the size of this string. It may be desirable to increase this for larger screens or trim it to increase gaps between hints.", OPT_STRING },
	{ "hint_font", "Arial", "The font name used by hints. Note: This is platform specific, in X it corresponds to a valid xft font name, on macos it corresponds to a postscript name.", OPT_STRING },

	{ "hint_size", "20", "Hint size (range: 1-1000)", OPT_INT },
	{ "hint_border_radius", "3", "Border radius.", OPT_INT },

	{ "hint_exit", "esc", "The exit key used for hint mode.", OPT_HINT_KEY },
	{ "hint_undo", "backspace", "undo last selection step in one of the hint based modes.", OPT_HINT_KEY },
	{ "hint_undo_all", "C-u", "undo all selection steps in one of the hint based modes.", OPT_HINT_KEY },

	{ "hint2_chars", "hjkl;asdfgqwertyuiopzxcvb", "The character set used for the second hint selection, should consist of at least hint_grid_size^2 characters.", OPT_STRING },
	{ "hint2_size", "20", "The size of hints in the secondary grid (range: 1-1000).", OPT_INT },
	{ "hint2_gap_size", "1", "The spacing between hints in the secondary grid. (range: 1-1000)", OPT_INT },
	{ "hint2_grid_size", "3", "The size of the secondary grid.", OPT_INT },

	{ "screen_chars", "jkl;asdfg", "The characters used for screen selection.", OPT_STRING },

	{ "scroll_speed", "300", "Initial scroll speed in units/second (unit varies by platform).", OPT_INT },
	{ "scroll_max_speed", "9000", "Maximum scroll speed.", OPT_INT },
	{ "scroll_acceleration", "1600", "Scroll acceleration in units/second^2.", OPT_INT },
	{ "scroll_deceleration", "-3400", "Scroll deceleration.", OPT_INT },

	{ "indicator", "none", "Specifies an optional visual indicator to be displayed while normal mode is active, must be one of: topright, topleft, bottomright, bottomleft, none", OPT_STRING },
	{ "indicator_color", "#00ff00", "The color of the visual indicator color.", OPT_STRING },
	{ "indicator_size", "12", "The size of the visual indicator in pixels.", OPT_INT },
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

enum option_type get_option_type(const char *key)
{
	size_t i;

	for (i = 0; i < sizeof(options) / sizeof(options[0]); i++) {
		if (!strcmp(options[i].key, key))
			return options[i].type;
	}

	fprintf(stderr, "ERROR: %s is not a valid config option\n", key);
	exit(-1);
}

static void validate_key_option(const char *s)
{
	struct input_event ev;
	char *tok;
	char buf[1024];

	strncpy(buf, s, sizeof buf);

	for (tok = strtok(buf, " "); tok; tok = strtok(NULL, " ")) {
		if (input_parse_string(&ev, tok)) {
			fprintf(stderr, "ERROR: %s is not a valid key name\n", tok);
			exit(-1);
		}
	}
}

static void config_add(const char *key, const char *val)
{
	struct config_entry *ent;
	ent = malloc(sizeof(struct config_entry));

	ent->key = key;
	ent->value = val;
	ent->type = get_option_type(key);

	switch (ent->type) {
		int i;

		case OPT_INT:
			for (i = 0; ent->value[i]; i++)
				if (!isdigit(ent->value[i]) && !(i == 0 && ent->value[0] == '-')) {
					fprintf(stderr, "ERROR: %s must be a valid int\n", ent->value);
					exit(-1);
				}
			break;
		case OPT_NORMAL_KEY:
		case OPT_ACTIVATION_KEY:
		case OPT_HINT_KEY:
		case OPT_GRID_KEY:
			validate_key_option(ent->value);

			break;

		default:
			break;

	}
	// check if key already exists
	// if so, replace value
	// if not, add to list
	int found = 0;
	struct config_entry *cur = config;
	while (cur) {
		if (!strcmp(cur->key, key)) {
			cur->value = val;
			free(ent);
			return;
		}
		cur = cur->next;
	}
	
	ent->next = config;

	config = ent;
}

void parse_config(const char *path)
{
	size_t i;

	FILE *fh = (path[0] == '-' && path[1] == 0) ? stdin : fopen(path, "r");

	for (i = 0; i < sizeof(options) / sizeof(options[0]); i++)
		config_add(options[i].key, options[i].val);

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

			if (!delim || line[0] == '#') {
				free(line);
				continue;
			}

			*delim = 0;
			while (*++delim == ' ');

			config_add(line, delim);
		}
		fclose(fh);
	}
}

/*
 * Consumes an input event and the name of a config option corresponding
 * to a set of keys and returns the 1-based index of the most recent
 * matching key (if any). The supplied config_key may be shadowed by
 * another key with the same option_type as the supplied key (in which
 * case this function will return 0).

 * NOTE: This is horribly inefficient (albeit fast enough). A better solution
 * would be to consume the event and type and return the corresponding
 * option for subsequent matching, but that would require
 * modifying all calling code.
 */

int config_input_match(struct input_event *ev, const char *config_key, int strict)
{
	char *tok;
	struct config_entry *ent;

	enum option_type type = get_option_type(config_key);

	if (type == OPT_STRING || type == OPT_INT) {
		fprintf(stderr, "ERROR: %s is not a valid key type\n", config_key);
		exit(-1);
	}

	for (ent = config; ent; ent = ent->next) {
		if (ent->type == type) {
			char buf[1024];
			snprintf(buf, sizeof buf, "%s", ent->value);

			int idx = 1;
			for (tok = strtok(buf, " "); tok; tok = strtok(NULL, " ")) {
				if (input_eq(ev, tok, strict)) {
					if (!strcmp(ent->key, config_key))
						return idx;
					else if (strict) //Shadowed by another config option
						return 0;
				}

				idx++;
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
