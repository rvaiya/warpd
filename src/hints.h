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

#ifndef __H_HINTS_
#define __H_HINTS_

#include <X11/X.h>
	
struct hint_keys {
	KeyCode up;
	KeyCode right;
	KeyCode left;
	KeyCode down;
	KeyCode buttons[8];
	KeyCode quit;
};

void hints(Display *_dpy, int _nc, int _nr, int inc, const char *bgcol, const char *fgcol, struct hint_keys *keys);
#endif
