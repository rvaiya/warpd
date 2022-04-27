/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "warpd.h"

struct {
	char *name;
	char *shifted_name;
} shift_table[] = {
    {"1", "!"}, {"2", "@"}, {"3", "#"}, {"4", "$"}, {"5", "%"}, {"6", "^"},
    {"7", "&"}, {"8", "*"}, {"9", "("}, {"0", ")"}, {"-", "_"}, {"=", "+"},
    {"q", "Q"}, {"w", "W"}, {"e", "E"}, {"r", "R"}, {"t", "T"}, {"y", "Y"},
    {"u", "U"}, {"i", "I"}, {"o", "O"}, {"p", "P"}, {"[", "{"}, {"]", "}"},
    {"a", "A"}, {"s", "S"}, {"d", "D"}, {"f", "F"}, {"g", "G"}, {"h", "H"},
    {"j", "J"}, {"k", "K"}, {"l", "L"}, {";", ":"}, {"`", "~"}, {"\\", "|"},
    {"z", "Z"}, {"x", "X"}, {"c", "C"}, {"v", "V"}, {"b", "B"}, {"n", "N"},
    {"m", "M"}, {",", "<"}, {".", ">"}, {"/", "?"},
};

static uint8_t cached_mods[256];

int input_parse_string(struct input_event *ev, const char *s)
{
	if (!s || s[0] == 0)
		return 0;

	ev->mods = 0;
	ev->pressed = 1;

	while (s[1] == '-') {
		switch (s[0]) {
		case 'A':
			ev->mods |= MOD_ALT;
			break;
		case 'M':
			ev->mods |= MOD_META;
			break;
		case 'S':
			ev->mods |= MOD_SHIFT;
			break;
		case 'C':
			ev->mods |= MOD_CONTROL;
			break;
		default:
			fprintf(stderr, "%s is not a valid s\n", s);
			exit(1);
		}

		s += 2;
	}

	if (s[0]) {
		size_t i;

		for (i = 0; i < sizeof(shift_table)/sizeof(shift_table[0]); i++)
			if (!strcmp(shift_table[i].shifted_name, s)) {
				s = shift_table[i].name;
				ev->mods |= MOD_SHIFT;
			}

		ev->code = input_lookup_code(s);

		if (!ev->code) {
			fprintf(stderr, "WARNING: %s is not a valid code!\n", s);
			return -1;
		}
	}

	return 0;
}

const char *input_event_tostr(struct input_event *ev)
{
	static char s[64];
	const char *name = input_lookup_name(ev->code);
	int n = 0;

	if (!ev)
		return "NULL";

	if (ev->mods & MOD_CONTROL) {
		s[n++] = 'C';
		s[n++] = '-';
	}

	if (ev->mods & MOD_SHIFT) {
		size_t i;
		int shifted = 0;

		for (i = 0; i < sizeof(shift_table) / sizeof(shift_table[0]); i++) {
			if (!strcmp(shift_table[i].name, name)) {
				shifted = 1;
				name = shift_table[i].shifted_name;
			}
		}

		if (!shifted) {
			s[n++] = 'S';
			s[n++] = '-';
		}
	}

	if (ev->mods & MOD_ALT) {
		s[n++] = 'A';
		s[n++] = '-';
	}

	if (ev->mods & MOD_META) {
		s[n++] = 'M';
		s[n++] = '-';
	}

	strcpy(s + n, name ? name : "UNDEFINED");

	return s;
}

int input_event_eq(struct input_event *ev, const char *str)
{
	uint8_t mods;
	struct input_event ev1;

	if (!ev)
		return 0;

	/*
	 * Cache mods on key down so we can properly detect the
	 * corresponding key up event in the case of intermittent
	 * modifier changes.
	 */
	if (ev->pressed) {
		mods = ev->mods;
		cached_mods[ev->code] = ev->mods;
	} else {
		mods = cached_mods[ev->code];
	}

	if (input_parse_string(&ev1, str) < 0)
		return 0;

	return ev1.code == ev->code && ev1.mods == mods;
}
