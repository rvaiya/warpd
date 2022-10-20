/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include "macos.h"

static void *mainloop(void *arg)
{
	void (*loop)() = (void(*)())arg;
	loop();

	exit(0);
}

static NSDictionary *get_font_attrs(const char *family, NSColor *color, int h)
{
	NSDictionary *attrs;

	int ptsz = h;
	CGSize size;
	do {
		NSFont *font =
		    [NSFont fontWithName:[NSString stringWithUTF8String:family]
				    size:ptsz];
		if (!font) {
			fprintf(stderr, "ERROR: %s is not a valid font\n",
				family);
			exit(-1);
		}
		attrs = @{
			NSFontAttributeName : font,
			NSForegroundColorAttributeName : color,
		};
		size = [@"m" sizeWithAttributes:attrs];
		ptsz--;
	} while (size.height > h);

	return attrs;
}

void macos_draw_text(struct screen *scr, NSColor *col, const char *font,
		     int x, int y, int w,
		     int h, const char *s)
{

	NSDictionary *attrs = get_font_attrs(font, col, h);
	NSString *str = [NSString stringWithUTF8String:s];
	CGSize size = [str sizeWithAttributes:attrs];

	x += (w - size.width)/2;

	y += size.height + (h - size.height)/2;

	/* Convert to LLO */
	y = scr->h - y;

	[str drawAtPoint:NSMakePoint((float)x, (float)y) withAttributes: attrs];
}

void macos_draw_box(struct screen *scr, NSColor *col, float x, float y, float w, float h, float r)
{
	[col setFill];

	/* Convert to LLO */
	y = scr->h - y - h;

	NSBezierPath *path = [NSBezierPath
	    bezierPathWithRoundedRect:NSMakeRect((float)x, (float)y, (float)w,
						 (float)h)
			      xRadius:(float)r
			      yRadius:(float)r];
	[path fill];
}


NSColor *nscolor_from_hex(const char *str)
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

void osx_copy_selection()
{
	int shifted;

	send_key(osx_input_lookup_code("leftmeta", &shifted), 1);
	send_key(osx_input_lookup_code("c", &shifted), 1);
	send_key(osx_input_lookup_code("leftmeta", &shifted), 0);
	send_key(osx_input_lookup_code("c", &shifted), 0);
}

void osx_scroll(int direction)
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

void osx_commit()
{
	dispatch_sync(dispatch_get_main_queue(), ^{
		size_t i;
		for (i = 0; i < nr_screens; i++) {
			struct window *win = screens[i].overlay;

			if (win->nr_hooks)
				window_show(win);
			else
				window_hide(win);
		}
	});
}

void osx_run(void (*loop)())
{
	pthread_t thread;

	[NSApplication sharedApplication];
	[NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];

	macos_init_input();
	macos_init_mouse();
	macos_init_screen();

	pthread_create(&thread, NULL, mainloop, (void *)loop);

	[NSApp run];
}

void platform_init()
{
	platform.commit = osx_commit;
	platform.copy_selection = osx_copy_selection;
	platform.hint_draw = osx_hint_draw;
	platform.init_hint = osx_init_hint;
	platform.input_grab_keyboard = osx_input_grab_keyboard;
	platform.input_lookup_code = osx_input_lookup_code;
	platform.input_lookup_name = osx_input_lookup_name;
	platform.input_next_event = osx_input_next_event;
	platform.input_ungrab_keyboard = osx_input_ungrab_keyboard;
	platform.input_wait = osx_input_wait;
	platform.mouse_click = osx_mouse_click;
	platform.mouse_down = osx_mouse_down;
	platform.mouse_get_position = osx_mouse_get_position;
	platform.mouse_hide = osx_mouse_hide;
	platform.mouse_move = osx_mouse_move;
	platform.mouse_show = osx_mouse_show;
	platform.mouse_up = osx_mouse_up;
	platform.run = osx_run;
	platform.screen_clear = osx_screen_clear;
	platform.screen_draw_box = osx_screen_draw_box;
	platform.screen_get_dimensions = osx_screen_get_dimensions;
	platform.screen_list = osx_screen_list;
	platform.scroll = osx_scroll;
}
