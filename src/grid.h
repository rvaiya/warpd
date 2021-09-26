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

#ifndef _H_GRID_
#define _H_GRID_

#define MAX_COLS 100
#define MAX_ROWS 100

#include <stdint.h>
#include "common.h"

struct grid_keys {
	uint16_t up;
	uint16_t right;
	uint16_t left;
	uint16_t down;

	uint16_t *exit;
	size_t exit_sz;
	uint16_t grid[MAX_ROWS*MAX_COLS];
};

uint16_t grid_mode(int startrow, int startcol);
void init_grid(Display *_dpy,
	       int _nr,
	       int _nc,
	       int _line_width,
	       int _cursor_width,
	       int _movement_increment,
	       const char *gridcol,
	       const char *mousecol,
	       struct grid_keys *_keys);
#endif
