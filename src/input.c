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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"

int input_parse_string(struct input_event *ev, const char *s)
{
	if(!s || s[0] == 0) 
		return 0;

	ev->mods = 0;
	ev->pressed = 1;

	while(s[1] == '-') {
		switch(s[0]) {
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

	if(s[0]) {
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
	int i = 0;

	if (!ev)
		return "NULL";

	if (ev->mods & MOD_CONTROL) {
		s[i++] = 'C';
		s[i++] = '-';
	}

	if (ev->mods & MOD_SHIFT) {
		s[i++] = 'S';
		s[i++] = '-';
	}

	if (ev->mods & MOD_ALT) {
		s[i++] = 'A';
		s[i++] = '-';
	}

	if (ev->mods & MOD_META) {
		s[i++] = 'M';
		s[i++] = '-';
	}


	strcpy(s+i, name ? name : "UNDEFINED");
	return s;
}

int input_event_eq(struct input_event *ev, const char *str)
{
	struct input_event ev1;

	if (!ev)
		return 0;

	if (input_parse_string(&ev1, str) < 0)
		return 0;

	return ev1.code == ev->code && 
		ev1.mods == ev->mods;
}
