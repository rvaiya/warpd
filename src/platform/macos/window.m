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

#include <Cocoa/Cocoa.h>
#include "impl.h"

struct window {
	NSWindow *win;
};

@interface MainView: NSView
@property void (*draw_fn)(NSView *view);
@end

@implementation MainView
/* Called by cocoa when the view needs to be redrawn. */
- (void)drawRect:(NSRect)dirtyRect {
	if (self.draw_fn)
		self.draw_fn(self);
}
@end

void window_show(struct window *win)
{
	dispatch_sync(dispatch_get_main_queue(), ^{
		[win->win makeKeyAndOrderFront:nil];
	});
}

void window_move(struct window *win, int x, int y)
{
	dispatch_sync(dispatch_get_main_queue(), ^{
		NSRect fr = [[NSScreen mainScreen] frame];

		const int sh = fr.size.height;
		const int sw = fr.size.width;
		const int wh = [win->win frame].size.height;

		int fy = sh-y-wh;
		int fx = x;

		fy = fy < 0 ? 0 : fy;
		fx = x > sw-wh ? sw-wh : x;

		[win->win makeKeyAndOrderFront:nil];
		[win->win setFrameOrigin:NSMakePoint(fx, fy)];
	});
}


void window_redraw(struct window *win)
{
	dispatch_sync(dispatch_get_main_queue(), ^{
		[[win->win contentView] setNeedsDisplay:TRUE];
	});
}

void window_hide(struct window *win)
{
	dispatch_sync(dispatch_get_main_queue(), ^{
		[win->win orderOut:nil];
	});
}

struct window *create_overlay_window(void (*draw_fn)(NSView *view))
{
	struct window *win = malloc(sizeof(struct window));

	dispatch_sync(dispatch_get_main_queue(), ^{
		NSScreen *scr = [NSScreen mainScreen];

		int sw = [scr frame].size.width;
		int sh = [scr frame].size.height;

		NSWindow *nsWin = [[NSWindow alloc]
		initWithContentRect:NSMakeRect(0, 0, sw, sh) //Initial size and position
			  styleMask:NSWindowStyleMaskBorderless
			    backing:NSBackingStoreBuffered
			      defer:FALSE];
		/*
		 * Make our custom view the main content view for the window.
		 * Doing this will create an instance of the view and give it
		 * the the frame (bounds) to that of the window. We can then use
		 * our overloaded drawRect method to draw to the window using
		 * the implicitly provided graphics context.
		 */

		MainView *view = [[MainView alloc] init];
		view.draw_fn = draw_fn;

		[nsWin setContentView:view];

		[nsWin setBackgroundColor:[NSColor clearColor]];
		[nsWin makeKeyAndOrderFront:nil];
		[nsWin setLevel:NSMainMenuWindowLevel+999];

		win->win = nsWin;
	});

	return win;
}

struct window *create_window(const char *color, size_t sz)
{
	struct window *win;
	NSRect rect = NSMakeRect(0, 0, sz, sz);
	NSColor *_color = parse_color(color);
	win = malloc(sizeof(struct window));

	dispatch_sync(dispatch_get_main_queue(), ^{
		NSWindow *nsWin = [[NSWindow alloc]
		initWithContentRect:rect
			  styleMask:NSWindowStyleMaskBorderless
			    backing:NSBackingStoreBuffered
			      defer:FALSE];

		[nsWin setBackgroundColor: _color];
		[nsWin setLevel:NSMainMenuWindowLevel+9000];
		[nsWin makeKeyAndOrderFront:nil];


	      win->win = nsWin;
	});


	return win;
}
