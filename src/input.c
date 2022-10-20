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
			fprintf(stderr, "%s is not a valid modifier\n", s);
			exit(-1);
		}

		s += 2;
	}

	if (s[0]) {
		int shifted;

		ev->code = platform.input_lookup_code(s, &shifted);
		if (shifted)
			ev->mods |= MOD_SHIFT;

		if (!ev->code)
			return -1;
	}

	return 0;
}

const char *input_event_tostr(struct input_event *ev)
{
	static char s[64];
	const char *name = platform.input_lookup_name(ev->code, ev->mods & MOD_SHIFT ? 1 : 0);
	int n = 0;

	if (!ev)
		return "NULL";

	if (ev->mods & MOD_CONTROL) {
		s[n++] = 'C';
		s[n++] = '-';
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

/*
 * Returns:
 * 0 on no match
 * 1 on code match
 * 2 on full match
 */
int input_eq(struct input_event *ev, const char *str)
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

	if (ev1.code != ev->code)
		return 0;
	else if (ev1.mods != mods)
		return 1;
	else
		return 2;
}
