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

#ifndef EVLOOP_H
#define EVLOOP_H

#include "../../platform.h"
#include <stdint.h>
#include <Cocoa/Cocoa.h>

struct window;

extern int hide_cursor;

void	window_show(struct window *win);
void	window_hide(struct window *win);
void	window_redraw(struct window *win);
void	window_move(struct window *win, int x, int y);

void	window_commit(struct window *w);

struct window	*create_overlay_window(void (*draw_fn)(NSView *view));
struct window	*create_window(const char *color, size_t sz);

void	grid_commit();
void	cursor_commit();

void	macos_init_input();
void	macos_init_mouse();

void	send_key(uint8_t code, int pressed);

NSColor *parse_color(const char *str);
#endif
