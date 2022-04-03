/*
 * warpd - A keyboard-driven modal pointer.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#ifndef EVLOOP_H
#define EVLOOP_H

#include "../../platform.h"
#include <Cocoa/Cocoa.h>
#include <stdint.h>

struct window;

extern int hide_cursor;

void window_show(struct window *win);
void window_hide(struct window *win);
void window_redraw(struct window *win);
void window_move(struct window *win, int x, int y);

void window_commit(struct window *w);

struct window *create_overlay_window(void (*draw_fn)(NSView *view));
struct window *create_window(const char *color, size_t sz);

void grid_commit();
void cursor_commit();

void macos_init_input();
void macos_init_mouse();

void send_key(uint8_t code, int pressed);

NSColor *parse_color(const char *str);
#endif
