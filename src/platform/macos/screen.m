/*
 * warpd - A keyboard-driven modal pointer.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include <Cocoa/Cocoa.h>

void screen_get_dimensions(int *w, int *h)
{
	NSRect frame = [[NSScreen mainScreen] frame];
	*w = frame.size.width;
	*h = frame.size.height;
}
