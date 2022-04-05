/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include "macos.h"

struct drawing_hook {
	void (*hook)(void *arg, NSView *view);
	void *arg;

	struct drawing_hook *next;
};

struct window {
	NSWindow *win;

	struct drawing_hook *hooks;
};

@interface MainView : NSView
@property struct window	*win;
@end

@implementation MainView
/* Called by cocoa when the view needs to be redrawn. */
- (void)drawRect:(NSRect)dirtyRect
{
	struct drawing_hook *hook;
	struct window *win = self.win;

	for (hook = win->hooks; hook; hook = hook->next)
		hook->hook(hook->arg, self);
}
@end

/* 
 * Wrap NSWindow in a nice C API, with a coordinate scheme based on the top
 * left, where God (and the X developers) intended it :)
 */
void window_show(struct window *win) 
{
	[win->win makeKeyAndOrderFront:nil];
	[[win->win contentView] setNeedsDisplay:TRUE];
}

void window_unregister_draw_fn(struct window *win, void (*draw)(void *arg, NSView *view), void *arg)
{

	struct drawing_hook **h = &win->hooks;
	while (*h) {
		if ((*h)->hook == draw && (*h)->arg == arg) {
			struct drawing_hook *tmp = *h;
			*h = (*h)->next;
			free(tmp);

			return;
		}
		h = &(*h)->next;
	}
}

void window_register_draw_fn(struct window *win, void (*draw)(void *arg, NSView *view), void *arg)
{
	struct drawing_hook **h = &win->hooks;

	while (*h) {
		if ((*h)->hook == draw && (*h)->arg == arg) {
			struct drawing_hook *tmp = *h;
			*h = (*h)->next;
			free(tmp);
		} else {
			h = &(*h)->next;
		}
	}

	*h = malloc(sizeof(struct drawing_hook));
	(*h)->hook = draw;
	(*h)->arg = arg;
	(*h)->next = NULL;
}

void window_move(struct window *win, struct screen *scr, int x, int y)
{
	const int wh = win->win.frame.size.height;
	[win->win setFrameOrigin:NSMakePoint(scr->x + x, scr->y + scr->h - wh - y)];
}

void window_hide(struct window *win) 
{
	[win->win orderOut:nil];
}

/* A fixed position, transparent, borderless window with the given dimensions and offset. */
struct window *create_overlay_window(int x, int y, int w, int h)
{
	struct window *win = calloc(1, sizeof(struct window));

	NSWindow *nsWin = [[NSWindow alloc]
	    initWithContentRect:NSMakeRect((float)x, (float)y, (float)w, (float)h) // Initial size and position
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
	view.win = win;

	[nsWin setContentView:view];

	[nsWin setBackgroundColor:[NSColor clearColor]];
	[nsWin makeKeyAndOrderFront:nil];
	[nsWin setLevel:NSMainMenuWindowLevel + 999];

	win->win = nsWin;

	return win;
}

struct window *create_window(const char *color, size_t w, size_t h)
{
	struct window	*win = calloc(1, sizeof(struct window));
	NSRect		rect = NSMakeRect(0, 0, (float)w, (float)h);

	NSWindow *nsWin = [[NSWindow alloc]
	    initWithContentRect:rect
		      styleMask:NSWindowStyleMaskBorderless
			backing:NSBackingStoreBuffered
			  defer:FALSE];

	[nsWin setBackgroundColor:nscolor_from_hex(color)];
	[nsWin setLevel:NSMainMenuWindowLevel + 9000];
	[nsWin makeKeyAndOrderFront:nil];

	win->win = nsWin;

	return win;
}
