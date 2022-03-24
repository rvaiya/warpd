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

#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stdlib.h>

#define MOD_CONTROL	1
#define MOD_SHIFT	2
#define MOD_META	4
#define MOD_ALT		8

#define SCROLL_DOWN	1
#define SCROLL_RIGHT	2
#define SCROLL_LEFT	3
#define SCROLL_UP	4

#define MAX_HINTS 9000

struct input_event {
	uint8_t code;
	uint8_t mods;
	uint8_t pressed;
};

struct hint {
	int x;
	int y;
	char label[16];
};


/* Main entry point for each platform. Must be called befor other functions are usable. */

void	start_main_loop(void (*init)(void));

/* Input */

void	 	 	 input_grab_keyboard();
void	 	 	 input_ungrab_keyboard();

struct input_event	*input_next_event(int timeout);

uint8_t			 input_lookup_code(const char *name);
const char		*input_lookup_name(uint8_t name);

/* 
 * Efficiently listen for one or more input events before
 * grabbing the keyboard (including the event itself)
 * and returning the matched event.
 */
struct input_event	*input_wait(struct input_event *events, size_t sz);


void	mouse_down(int btn);
void	mouse_up(int btn);

void	mouse_move(int x, int y);
void	mouse_click(int btn);
void	mouse_get_position(int *x, int *y);
void	mouse_show();
void	mouse_hide();

/* Cursor */

void	init_cursor(const char *color, size_t sz);

void	cursor_hide();
void	cursor_show(int x, int y);

/* Grid */
struct grid;

void		 grid_draw(struct grid *g, int x, int y, int w, int h);
void		 grid_hide(struct grid *g);
struct grid	*create_grid(const char *color, size_t width, size_t nc, size_t nr);

/* Util */

void	screen_get_dimensions(int *sw, int *sh);

/* Hints */

/* Hints are centered around the provided x,y coordinates. */
void init_hint(struct hint *_hints, size_t n, int _box_height, int _border_radius, 
			const char *bg, const char *fg, const char *font_family);

/* indices must be the same size as the initialized hints */
void	hint_show(uint8_t *indices);
void	hint_hide();


void	scroll(int direction);

void	copy_selection();
#endif
