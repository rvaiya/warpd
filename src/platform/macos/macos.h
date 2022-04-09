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
#endif
