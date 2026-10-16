/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include "macos.h"

static NSTimer *hider_timer = NULL;
static int hide_depth = 0;
static int dragging = 0;

@interface CursorHider : NSObject
@end

@implementation CursorHider
- (void)hide
{
	CGDisplayHideCursor(kCGDirectMainDisplay);
	hide_depth++;
}
@end

static CursorHider *hider = NULL;

static long get_time_ms()
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	return ts.tv_sec * 1E3 + ts.tv_nsec / 1E6;
}

static void do_mouse_click(int btn, int pressed, int nclicks)
{
	CGEventRef ev = CGEventCreate(NULL);
	CGPoint current_pos = CGEventGetLocation(ev);
	CFRelease(ev);

	int down = kCGEventLeftMouseDown;
	int up = kCGEventLeftMouseUp;
	int button = kCGMouseButtonLeft;
	CGEventFlags mask = 0;

	switch (btn) {
	case 3:
		down = kCGEventRightMouseDown;
		up = kCGEventRightMouseUp;
		button = kCGMouseButtonRight;
		break;
	case 1:
		down = kCGEventLeftMouseDown;
		up = kCGEventLeftMouseUp;
		button = kCGMouseButtonLeft;
		break;
	default:
		down = kCGEventOtherMouseDown;
		up = kCGEventOtherMouseUp;
		button = btn;
		break;
	}

	if (active_mods & PLATFORM_MOD_META) mask |= kCGEventFlagMaskCommand;
	if (active_mods & PLATFORM_MOD_ALT) mask |= kCGEventFlagMaskAlternate;
	if (active_mods & PLATFORM_MOD_CONTROL) mask |= kCGEventFlagMaskControl;
	if (active_mods & PLATFORM_MOD_SHIFT) mask |= kCGEventFlagMaskShift;

	if (pressed) {
		ev = CGEventCreateMouseEvent(NULL, down, current_pos, button);

		CGEventSetFlags(ev, mask);

		CGEventSetIntegerValueField(ev, kCGMouseEventClickState,
					    nclicks);
		CGEventPost(kCGHIDEventTap, ev);
		CFRelease(ev);
	} else {
		ev = CGEventCreateMouseEvent(NULL, up, current_pos, button);

		CGEventSetFlags(ev, mask);

		CGEventSetIntegerValueField(ev, kCGMouseEventClickState,
					    nclicks);
		CGEventPost(kCGHIDEventTap, ev);
		CFRelease(ev);
	}
}

void osx_mouse_click(int btn)
{
	dispatch_sync(dispatch_get_main_queue(), ^{
		const int threshold = 300;
		dragging = 0;

		static long last_ts = 0;
		static int clicks = 1;

		/*
		 * Apparently quartz events accrete and encode the number of clicks
		 * rather than leaving this to the application, so we need this ugly
		 * workaround :/.
		 */

		if ((get_time_ms() - last_ts) < threshold)
			clicks++;
		else
			clicks = 1;

		do_mouse_click(btn, 1, clicks);
		do_mouse_click(btn, 0, clicks);

		last_ts = get_time_ms();
	});
}

void osx_mouse_up(int btn)
{
	if (btn == 1)
		dragging = 0;

	do_mouse_click(btn, 0, 1);
}

void osx_mouse_down(int btn)
{
	if (btn == 1)
		dragging = 1;

	do_mouse_click(btn, 1, 1);
}

void osx_mouse_get_position(struct screen **_scr, int *_x, int *_y)
{
	size_t i;
	NSPoint loc = [NSEvent mouseLocation];
	int x = loc.x;
	int y = loc.y;

	struct screen *scr = &screens[0];
	long best = -1;

	/*
	 * Pick the screen containing the pointer, or failing that (e.g the
	 * display configuration changed under us), the nearest one.
	 */
	for (i = 0; i < nr_screens; i++) {
		long dx = 0, dy = 0, dist;

		if (x < screens[i].x)
			dx = screens[i].x - x;
		else if (x > screens[i].x + screens[i].w)
			dx = x - (screens[i].x + screens[i].w);

		if (y < screens[i].y)
			dy = screens[i].y - y;
		else if (y > screens[i].y + screens[i].h)
			dy = y - (screens[i].y + screens[i].h);

		dist = dx*dx + dy*dy;
		if (best < 0 || dist < best) {
			best = dist;
			scr = &screens[i];
		}

		if (dist == 0)
			break;
	}

	x -= scr->x;
	y -= scr->y;

	y = scr->h - y;

	/* Clamp in case we fell back to a screen which doesn't contain the pointer. */
	x = x < 0 ? 0 : (x > scr->w ? scr->w : x);
	y = y < 0 ? 0 : (y > scr->h ? scr->h : y);

	if (_x)
		*_x = x;
	if (_y)
		*_y = y;
	if (_scr)
		*_scr = scr;
}

void osx_mouse_move(struct screen *scr, int x, int y)
{
	const int type = dragging ? kCGEventLeftMouseDragged : kCGEventMouseMoved;
	int cgx, cgy;

	x += scr->x;
	y = scr->y + scr->h - y;

	/*
	 * CGEvents use a different coordinate system, so we have to convert between the
	 * two.
	 */

	NSPoint nspos = [NSEvent mouseLocation]; //LLO global coordinate system
	CGEventRef CGEv = CGEventCreate(NULL);
	CGPoint cgpos = CGEventGetLocation(CGEv); //ULO global coordinate system :(

	cgx = x - nspos.x + cgpos.x;
	cgy = cgpos.y - (y - nspos.y);

	CGEventRef ev =
	    CGEventCreateMouseEvent(NULL, type, CGPointMake(cgx, cgy), 0);
	CGEventPost(kCGHIDEventTap, ev);
	CFRelease(ev);
}

void osx_mouse_hide()
{
	dispatch_sync(dispatch_get_main_queue(), ^{
		if (hider_timer)
			return;

		/*
		 * Our kludge only works until the mouse is placed over the dock
		 * or system toolbar, so we have to keep hiding the cursor :(.
		 */
		hider_timer =
		    [NSTimer scheduledTimerWithTimeInterval:0.001
						     target:[CursorHider alloc]
						   selector:@selector(hide)
						   userInfo:nil
						    repeats:true];
	});
}

void osx_mouse_show()
{
	dispatch_sync(dispatch_get_main_queue(), ^{
		int i;

		if (!hider_timer)
			return;

		[hider_timer invalidate];
		hider_timer = NULL;

		/* :( */
		for (i = 0; i < hide_depth; i++)
			CGDisplayShowCursor(kCGDirectMainDisplay);

		hide_depth = 0;
	});
}

void macos_init_mouse()
{
	/*
	 * Kludge to make background cursor setting work.
	 *
	 * Adapted from
	 * http://web.archive.org/web/20150609013355/http://lists.apple.com:80/archives/carbon-dev/2006/Jan/msg00555.html
	 */

	void CGSSetConnectionProperty(int, int, CFStringRef, CFBooleanRef);
	int _CGSDefaultConnection();
	CFStringRef propertyString;

	propertyString = CFStringCreateWithCString(
	    NULL, "SetsCursorInBackground", kCFStringEncodingUTF8);
	CGSSetConnectionProperty(_CGSDefaultConnection(),
				 _CGSDefaultConnection(), propertyString,
				 kCFBooleanTrue);
	CFRelease(propertyString);

	hider = [CursorHider alloc];
}
