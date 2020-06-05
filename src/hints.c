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

#include <X11/extensions/Xinerama.h>
#include <X11/extensions/XTest.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "hints.h"
#include "input.h"
#include "dbg.h"
#include "hint_drw.h"

static size_t nhints = 0;
static struct hint hints[MAX_HINTS];
static struct hint_keys *keys;
static Display *dpy;

static const char *normalize(const char *s)
{
	if(!strcmp(s, "apostrophe"))
		return "'";
	else if(!strcmp(s, "period"))
		return ".";
	else if(!strcmp(s, "slash"))
		return "/";
	else if(!strcmp(s, "comma"))
		return ",";
	else if(!strcmp(s, "semicolon"))
		return ";";
	else
		return s;
}

static void generate_positions(struct hint *hints, size_t n, int w, int h) 
{
	XWindowAttributes info;
	size_t i, nr, nc;

	if(!n) {
		fprintf(stderr, "FATAL: Cannot generate hints from empty hint set (try populating or removing ~/.warprc_hints\n");
		exit(-1);
	}
	for (nc = 0; (nc*nc) < n; nc++);
	nr = nc;

	XGetWindowAttributes(dpy, DefaultRootWindow(dpy), &info);

	const float colw = info.width / nc;
	const float rowh = info.height / nr;

	for (i = 0; i < n; i++) {
		hints[i].x = ((i % nc) * colw) + ((colw-w)/2);
		hints[i].y = ((i / nc) * rowh) + ((rowh-h)/2);
		hints[i].w = w;
		hints[i].h = h;
	}
}

static size_t read_hints(const char *path,
			 struct hint *hints,
			 int w, int h) 
{
	size_t l = 0, i = 0;
	ssize_t len = 0;
	char *line = NULL;
	FILE *fh = fopen(path, "r");
	if(!fh) {
		perror("fopen");
		exit(-1);
	}

	while((len=getline(&line, &l, fh)) != -1) {
		line[len-1] = '\0';

		assert(i < MAX_HINTS);
		assert(len < sizeof hints[0].label);

		strcpy(hints[i++].label, line);
		free(line);

		line = NULL;
		l = 0;
	}


	generate_positions(hints, i, w, h);
	return i;
}


static size_t generate(const char *hint_characters,
		       int w,
		       int h,
		       struct hint hints[MAX_HINTS]) 
{
	size_t i, j;
	size_t n = strlen(hint_characters);

	for (i = 0; i < n; i++)
		for (j = 0; j < n; j++) {
			hints[i*n+j].label[0] = hint_characters[i];
			hints[i*n+j].label[1] = hint_characters[j];
			hints[i*n+j].label[2] = '\0';
		}

	generate_positions(hints, n*n, w, h);
	return n*n;
}

static int lbl_cmp(const char *s, const char *prefix)
{
		while(*prefix && *s) {
			if(*s == ' ') { 
				s++;
				continue;
			}

			if(*prefix != *s)
				return -1;

			prefix++;
			s++;
		}

		return 0;
}

static int filter(const char *prefix, struct hint **target)
{
	size_t i;
	size_t n = 0;
	static size_t indices[MAX_HINTS];

	if(!prefix) {
		hint_drw_filter(NULL, 0);
		return nhints;
	}

	if(target) *target = NULL;

	for (i = 0; i < nhints; i++) {
		if(!lbl_cmp(hints[i].label, prefix)) indices[n++] = i;
	}

	hint_drw_filter(indices, n);

	if(n == 1 && target)
		*target = &hints[indices[0]]; 

	return n;
}

uint16_t hint_warp()
{
	char buf[256];
	const uint16_t backspace = input_parse_keyseq("BackSpace");

	buf[0] = '\0';
	filter("", NULL);
	while(1) {
		size_t n;
		struct hint *target = NULL;
		const int keyseq = input_next_key(0);

		if(keyseq == backspace) {
			if(buf[0] != '\0')
				buf[strlen(buf)-1] = '\0';
		} else {
			strcpy(buf + strlen(buf), normalize(input_keyseq_to_string(keyseq)));
		}

		n = filter(buf, &target);
		if(target) {
			filter(NULL, NULL);
			XWarpPointer(dpy,
				     0, DefaultRootWindow(dpy),
				     0, 0, 0, 0,
				     target->x+target->w/2,
				     target->y+target->h/2);
			XFlush(dpy);
			return 0;
		} else if(!n) {
			return keyseq;
		}
	}
}

void init_hint(Display *_dpy,
	       char *hint_characters,
	       const char *bgcol,
	       const char *fgcol,
	       int width,
	       int height,
	       int opacity,
	       struct hint_keys *_keys) 
{
	dpy = _dpy;
	keys = _keys;

	char path[PATH_MAX];
	sprintf(path, "%s/%s", getenv("HOME"), ".warprc_hints");

	if(!access(path, F_OK)) {
		dbg("Found .warprc_hints, attempting to generate hints from it...");
		nhints = read_hints(path, hints, width, height) ;
	} else
		nhints = generate(hint_characters, width, height, hints);

	init_hint_drw(dpy, hints, nhints, opacity, bgcol, fgcol);
}
