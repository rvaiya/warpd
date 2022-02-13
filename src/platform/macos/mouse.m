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

#include <stdio.h>
#include <stddef.h>

#include "impl.h"

#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>

static NSTimer *hider_timer = NULL;
static int hide_depth = 0;
static int dragging = 0;

@interface CursorHider: NSObject
@end

@implementation CursorHider
- (void)hide {
	CGDisplayHideCursor(kCGDirectMainDisplay);
	hide_depth++;
}
@end

static CursorHider *hider = NULL;

static long get_time_ms()
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	return ts.tv_sec*1E3+ts.tv_nsec/1E6;
}

static void do_mouse_click (int btn, int pressed, int nclicks)
{
	CGEventRef ev = CGEventCreate(NULL);
	CGPoint current_pos = CGEventGetLocation(ev);
	CFRelease(ev);

	int down = kCGEventLeftMouseDown;
	int up = kCGEventLeftMouseUp;
	int button = kCGMouseButtonLeft;

	/* TODO: Add support for middle click. */
	if (btn == 3) {
		down = kCGEventRightMouseDown;
		up = kCGEventRightMouseUp;
		button = kCGMouseButtonRight;
	}


	if (pressed) {
		ev = CGEventCreateMouseEvent(NULL, down, current_pos, button);
		CGEventSetIntegerValueField(ev, kCGMouseEventClickState, nclicks);
		CGEventPost(kCGHIDEventTap, ev);
		CFRelease(ev);
	} else {
		ev = CGEventCreateMouseEvent(NULL, up, current_pos, button);
		CGEventSetIntegerValueField(ev, kCGMouseEventClickState, nclicks);
		CGEventPost(kCGHIDEventTap, ev);
		CFRelease(ev);
	}
}

void mouse_click(int btn)
{
	const int threshold = 300;
	dragging = 0;

	static long last_ts = 0;
	static int clicks = 1;

	/* 
	 * Apparently quartz events accrete and encode the number of clicks
	 * rather than leaving this to the application, so we need this ugly
	 * workaround :/.
	 */

	if ((get_time_ms()-last_ts) < threshold)
		clicks++;
	else
		clicks = 1;

	do_mouse_click(btn, 1, clicks);
	do_mouse_click(btn, 0, clicks);

	last_ts = get_time_ms();
}

void mouse_up(int btn)
{
	if (btn == 1)
		dragging = 0;

	do_mouse_click(btn, 0, 1);
}

void mouse_down(int btn)
{
	if (btn == 1)
		dragging = 1;

	do_mouse_click(btn, 1, 1);
}

void mouse_get_position(int *x, int *y)
{
	CGEventRef CGEv = CGEventCreate(NULL);
	CGPoint current_pos = CGEventGetLocation(CGEv);

	*x = current_pos.x;
	*y = current_pos.y;

	CFRelease(CGEv);
}

void mouse_move(int x, int y)
{
	const int type = dragging ? kCGEventLeftMouseDragged : kCGEventMouseMoved;
	int sw, sh;

	screen_get_dimensions(&sw, &sh);

	if (y > sh)
		y = sh;
	if (x > sw)
		x = sw;
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;

	
	CGEventRef ev = CGEventCreateMouseEvent(NULL, type, CGPointMake(x, y), 0);
	CGEventPost(kCGHIDEventTap, ev);  
	CFRelease(ev);
}

void mouse_hide()
{
	/* Our kludge only works until the mouse is placed over the dock or system toolbar, so we have to keep hiding the cursor :(. */
	dispatch_sync(dispatch_get_main_queue(), ^{
		if (hider_timer)
			return;
		hider_timer = [NSTimer scheduledTimerWithTimeInterval:0.001 target: [CursorHider alloc] selector: @selector(hide) userInfo: nil repeats: true ];
	});
}

void mouse_show()
{
	dispatch_sync(dispatch_get_main_queue(), ^{
		int i;

		if (!hider_timer)
			return;

		[hider_timer invalidate];
		hider_timer = NULL;

		/* :( */
		for (i = 0; i < hide_depth;i++)
			CGDisplayShowCursor(kCGDirectMainDisplay);

		hide_depth = 0;
	});
}

void init_mouse()
{
	/* 
	 * Kludge to make background cursor setting work.
	 *
	 * Adapted from http://web.archive.org/web/20150609013355/http://lists.apple.com:80/archives/carbon-dev/2006/Jan/msg00555.html
	 */

	void CGSSetConnectionProperty(int, int, CFStringRef, CFBooleanRef);
	int _CGSDefaultConnection();
	CFStringRef propertyString;

	propertyString = CFStringCreateWithCString(NULL, "SetsCursorInBackground", kCFStringEncodingUTF8);
	CGSSetConnectionProperty(_CGSDefaultConnection(), _CGSDefaultConnection(), propertyString, kCFBooleanTrue);
	CFRelease(propertyString);

	hider = [CursorHider alloc];
}
