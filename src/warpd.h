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

#ifndef WARPD
#define WARPD

#include "cfg.h"
#include "platform.h"
#include <time.h>

extern struct cfg *cfg;

enum {
	MODE_RESERVED,

	MODE_HINT,
	MODE_GRID,
	MODE_NORMAL,
};

void			 screen_get_dimensions(int *x, int *y);

struct input_event	*normal_mode(struct input_event *start_ev);
struct input_event	*grid_mode();
int			 hint_mode();

void	init_hint_mode();
void	init_normal_mode();
void	init_grid_mode();

const char	*input_event_tostr(struct input_event *ev);
int		 input_event_eq(struct input_event *ev, const char *str);
int		 input_parse_string(struct input_event *ev, const char *s);

void	toggle_drag();

int	mouse_process_key(struct input_event *ev, 
			const char *up_key, const char *down_key,
			const char *left_key, const char *right_key);

void	mouse_reset();

void	scroll_tick();
void	scroll_stop();
void	scroll_accelerate(int direction);
void	scroll_decelerate();

int	hist_add(int x, int y);
int	hist_get(int *x, int *y);
void	hist_prev();
void	hist_next();

#endif
