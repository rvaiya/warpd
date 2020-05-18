/*
 * ---------------------------------------------------------------------
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 * Original Author: Raheman Vaiya
 * ---------------------------------------------------------------------
 */

#ifndef _H_GRID_
#define _H_GRID_

#define MAX_COLS 100
#define MAX_ROWS 100

#include <X11/X.h>
#include <X11/Xlib.h>
#include "grid.h"

struct grid_keys {
	KeyCode up;
	KeyCode right;
	KeyCode left;
	KeyCode down;

	KeyCode buttons[8];

	KeyCode close_key;

	KeyCode grid[MAX_ROWS*MAX_COLS];
};

void grid(Display *_dpy, int _nr, int _nc, int movement_increment, int startrow, int startcol, const char *gridcol, const char *mousecol, struct grid_keys *keys);
#endif
