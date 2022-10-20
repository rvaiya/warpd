/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#ifndef X_PLATFORM_H
#define X_PLATFORM_H

#include "../../../platform.h"
#include "../../../warpd.h"

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
void init_xscreens();

/* Globals. */
extern Display *dpy;

extern struct screen xscreens[32];
extern size_t nr_xscreens;
extern uint8_t x_active_mods;

void x_run(void (*init)(void));
void x_input_grab_keyboard();
void x_input_ungrab_keyboard();
struct input_event *x_input_next_event(int timeout);
uint8_t x_input_lookup_code(const char *name, int *shifted);
const char *x_input_lookup_name(uint8_t code, int shifted);
struct input_event *x_input_wait(struct input_event *events, size_t sz);
void x_mouse_move(screen_t scr, int x, int y);
void x_mouse_down(int btn);
void x_mouse_up(int btn);
void x_mouse_click(int btn);
void x_mouse_get_position(screen_t *scr, int *x, int *y);
void x_mouse_show();
void x_mouse_hide();
void x_screen_get_dimensions(screen_t scr, int *w, int *h);
void x_screen_draw_box(screen_t scr, int x, int y, int w, int h, const char *color);
void x_screen_clear(screen_t scr);
void x_screen_list(screen_t scr[MAX_SCREENS], size_t *n);
void x_init_hint(const char *bg, const char *fg, int border_radius, const char *font_family);
void x_hint_draw(struct screen *scr, struct hint *hints, size_t n);
void x_scroll(int direction);
void x_copy_selection();
void x_commit();

#endif
