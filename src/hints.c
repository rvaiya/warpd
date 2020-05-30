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

static size_t generate(Display *dpy,
			     const char *hint_characters,
			     int w,
			     int h,
			     struct hint hints[MAX_HINTS]) 
{
	size_t i, j;
	XWindowAttributes info;
	XGetWindowAttributes(dpy, DefaultRootWindow(dpy), &info);

	const size_t nr = strlen(hint_characters);
	const size_t nc = strlen(hint_characters);

	const int rowh = info.height / nc;
	const int colw = info.width / nr;

	assert(nr * nc < MAX_HINTS);

	for (i = 0; i < nr; i++)
		for (j = 0; j < nc; j++) {
			int idx = i*nc+j;

			hints[idx].label[0] = hint_characters[i];
			hints[idx].label[1] = hint_characters[j];
			hints[idx].label[2] = '\0';

			hints[idx].x = colw * j + (colw - w)/2;
			hints[idx].y = rowh * i + (rowh - h)/2;
			hints[idx].w = w;
			hints[idx].h = h;
		}

	return nc * nr;
}

static int filter(const char *s, struct hint **target)
{
	size_t i;
	size_t n = 0;
	static size_t indices[MAX_HINTS];

	if(target) *target = NULL;

	for (i = 0; i < nhints; i++) {
		if(s && strstr(hints[i].label, s) == hints[i].label)
			indices[n++] = i;
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
	nhints = generate(dpy, hint_characters, width, height, hints);
	init_hint_drw(dpy, hints, nhints, opacity, bgcol, fgcol);
}
