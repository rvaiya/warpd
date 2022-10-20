/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#ifndef EVLOOP_H
#define EVLOOP_H

#include "../../platform.h"
#include <ApplicationServices/ApplicationServices.h>
#include <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>
#include <stdint.h>
#include <pthread.h>

/* 
 * NOTE: our API uses an upper left origin (ULO) coordinate system, while
 * most* of the macOS APIs use a lower left origin (LLO) coordinate system.
 * Translation between the coordinate systems should be done at the latest
 * point possible (i.e before the corresponding NS/CG calls). This is expected
 * to be understood implicitly when reading platform code.
 * 
 * * To compound matters, it seems some of the CG APIs use an ULO coordinate
 * system :/. When in doubt, consult docs from the high tower.
 */

#define MAX_DRAWING_HOOKS 32
#define MAX_BOXES 32

struct box {
	int x;
	int y;
	int w;
	int h;

	struct screen *scr;
	NSColor *color;
};

size_t nr_boxes;

struct drawing_hook {
	void (*hook)(void *arg, NSView *view);
	void *arg;
};

struct window {
	NSWindow *win;

	size_t nr_hooks;
	struct drawing_hook hooks[MAX_DRAWING_HOOKS];
};

struct screen {
	/* LLO */
	int x;
	int y;

	int w;
	int h;

	struct hint hints[MAX_HINTS];
	size_t nr_hints;

	struct box boxes[MAX_BOXES];
	size_t nr_boxes;

	struct window *overlay;

};


void window_show(struct window *win);
void window_hide(struct window *win);
void window_move(struct window *win, struct screen *scr, int x, int y);

void window_register_draw_hook(struct window *win,
			       void (*draw)(void *arg, NSView *view),
			       void *arg);

struct window *create_window(const char *color, size_t w, size_t h);
struct window *create_overlay_window();

void macos_init_input();
void macos_init_mouse();
void macos_init_screen();

void macos_draw_box(struct screen *scr, NSColor *col, float x, float y, float w,
		    float h, float r);

void macos_draw_text(struct screen *scr, NSColor *col, const char *font, int x,
		     int y, int w, int h, const char *s);

void send_key(uint8_t code, int pressed);

NSColor *nscolor_from_hex(const char *str);

extern struct screen screens[32];
extern size_t nr_screens;
extern uint8_t active_mods;

void osx_run(void (*init)(void));
void osx_input_grab_keyboard();
void osx_input_ungrab_keyboard();
struct input_event *osx_input_next_event(int timeout);
uint8_t osx_input_lookup_code(const char *name, int *shifted);
const char *osx_input_lookup_name(uint8_t code, int shifted);
struct input_event *osx_input_wait(struct input_event *events, size_t sz);
void osx_mouse_move(screen_t scr, int x, int y);
void osx_mouse_down(int btn);
void osx_mouse_up(int btn);
void osx_mouse_click(int btn);
void osx_mouse_get_position(screen_t *scr, int *x, int *y);
void osx_mouse_show();
void osx_mouse_hide();
void osx_screen_get_dimensions(screen_t scr, int *w, int *h);
void osx_screen_draw_box(screen_t scr, int x, int y, int w, int h, const char *color);
void osx_screen_clear(screen_t scr);
void osx_screen_list(screen_t scr[MAX_SCREENS], size_t *n);
void osx_init_hint(const char *bg, const char *fg, int border_radius, const char *font_family);
void osx_hint_draw(struct screen *scr, struct hint *hints, size_t n);
void osx_scroll(int direction);
void osx_copy_selection();
void osx_commit();

#endif
