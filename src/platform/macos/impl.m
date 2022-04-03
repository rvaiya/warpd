/*
 * warpd - A keyboard-driven modal pointer.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#import <Cocoa/Cocoa.h>
#import <assert.h>

#include "../../platform.h"
#include "impl.h"

int hide_cursor = 0;

void *mainloop(void *arg)
{
	((void (*)())arg)();

	exit(0);
}

NSColor *parse_color(const char *str)
{
	ssize_t len;
	uint8_t r, g, b, a;
#define X2B(c) ((c >= '0' && c <= '9') ? (c & 0xF) : (((c | 0x20) - 'a') + 10))

	if (str == NULL)
		return 0;

	str = (*str == '#') ? str + 1 : str;
	len = strlen(str);

	if (len != 6 && len != 8) {
		fprintf(stderr, "Failed to parse %s, paint it black!\n", str);
		return NSColor.blackColor;
	}

	r = X2B(str[0]);
	r <<= 4;
	r |= X2B(str[1]);

	g = X2B(str[2]);
	g <<= 4;
	g |= X2B(str[3]);

	b = X2B(str[4]);
	b <<= 4;
	b |= X2B(str[5]);

	a = 255;
	if (len == 8) {
		a = X2B(str[6]);
		a <<= 4;
		a |= X2B(str[7]);
	}

	return [NSColor colorWithCalibratedRed:(float)r / 255
					 green:(float)g / 255
					  blue:(float)b / 255
					 alpha:(float)a / 255];
}

void copy_selection()
{
	send_key(input_lookup_code("leftmeta"), 1);
	send_key(input_lookup_code("c"), 1);
	send_key(input_lookup_code("leftmeta"), 0);
	send_key(input_lookup_code("c"), 0);
}

void scroll(int direction)
{
	int y = 0;
	int x = 0;

	switch (direction) {
	case SCROLL_UP:
		y = 1;
		break;
	case SCROLL_DOWN:
		y = -1;
		break;
	case SCROLL_RIGHT:
		x = -1;
		break;
	case SCROLL_LEFT:
		x = 1;
		break;
	}

	CGEventRef ev = CGEventCreateScrollWheelEvent(
	    NULL, kCGScrollEventUnitPixel, 2, y, x);
	CGEventPost(kCGHIDEventTap, ev);
}

void platform_commit()
{
	grid_commit();
	cursor_commit();
}

void start_main_loop(void (*loop)())
{
	pthread_t thread;

	macos_init_input();
	macos_init_mouse();

	[NSApplication sharedApplication];
	[NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];

	pthread_create(&thread, NULL, mainloop, (void *)loop);

	[NSApp run];
}
