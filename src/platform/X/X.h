/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#ifndef X_PLATFORM_H
#define X_PLATFORM_H

#include "../../platform.h"
#include "../../warpd.h"

#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xinerama.h>
#include <X11/extensions/shape.h>
#include <X11/keysym.h>
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define MAX_BOXES 64

struct box {
	Window win;
	char color[32];
	int mapped;
};

struct screen {
	/* Xinerama offset */
	int x;
	int y;

	int w;
	int h;

	Pixmap buf;

	Window hintwin;

	Window cached_hintwin;
	Pixmap cached_hintbuf;

	struct hint cached_hints[MAX_HINTS];
	size_t nr_cached_hints;

	struct box boxes[MAX_BOXES];
	size_t nr_boxes;
};

Window create_window(const char *color);

int hex_to_rgba(const char *str, uint8_t *r, uint8_t *g, uint8_t *b,
		     uint8_t *a);
uint32_t parse_xcolor(const char *s, uint8_t *opacity);
void init_screens();

/* Globals. */
extern Display *dpy;

extern struct screen screens[32];
extern size_t nr_screens;
extern uint8_t active_mods;

#endif
