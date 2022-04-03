/*
 * warpd - A keyboard-driven modal pointer.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include "impl.h"
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/Xfixes.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int mouse_visible = 1;
Display *  dpy = NULL;

static uint32_t xcolor(uint8_t red, uint8_t green, uint8_t blue)
{
	XColor col;
	col.red = (int)red << 8;
	col.green = (int)green << 8;
	col.blue = (int)blue << 8;
	col.flags = DoRed | DoGreen | DoBlue;

	assert(
	    XAllocColor(dpy, XDefaultColormap(dpy, DefaultScreen(dpy)), &col));
	return col.pixel;
}

int hex_to_rgba(const char *str, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a)
{
#define X2B(c) ((c >= '0' && c <= '9') ? (c & 0xF) : (((c | 0x20) - 'a') + 10))

	if (str == NULL)
		return 0;
	str = (*str == '#') ? str + 1 : str;

	ssize_t len = strlen(str);

	if (len != 6 && len != 8)
		return -1;

	*r = X2B(str[0]);
	*r <<= 4;
	*r |= X2B(str[1]);

	*g = X2B(str[2]);
	*g <<= 4;
	*g |= X2B(str[3]);

	*b = X2B(str[4]);
	*b <<= 4;
	*b |= X2B(str[5]);

	*a = 255;
	if (len == 8) {
		*a = X2B(str[6]);
		*a <<= 4;
		*a |= X2B(str[7]);
	}

	return 0;
}

void screen_get_dimensions(int *sw, int *sh)
{
	*sw = WidthOfScreen(DefaultScreenOfDisplay(dpy));
	*sh = HeightOfScreen(DefaultScreenOfDisplay(dpy));
}

/*
 * Disable shadows for compton based compositors.
 *
 * Ref:
 * https://github.com/yshui/picom/blob/aa316aa3601a4f3ce9c1ca79932218ab574e61a7/src/win.c#L850
 */
static void disable_compton_shadow(Display *dpy, Window w)
{
	Atom _COMPTON_SHADOW = XInternAtom(dpy, "_COMPTON_SHADOW", False);
	unsigned int v = 0;

	XChangeProperty(dpy, w, _COMPTON_SHADOW, XA_CARDINAL, 32,
			PropModeReplace, (unsigned char *)&v, 1L);
}

static void set_opacity(Display *dpy, Window w, uint8_t _opacity)
{
	Atom OPACITY_ATOM = XInternAtom(dpy, "_NET_WM_WINDOW_OPACITY", False);

	unsigned int opacity =
	    (unsigned int)(((double)_opacity / 255) * (double)0xffffffff);

	XChangeProperty(dpy, w, OPACITY_ATOM, XA_CARDINAL, 32, PropModeReplace,
			(unsigned char *)&opacity, 1L);
}

void pixmap_copy(struct pixmap *pixmap, Window win)
{
	XCopyArea(dpy, pixmap->map, win, pixmap->gc, 0, 0, pixmap->w, pixmap->h,
		  0, 0);
}

void copy_selection()
{
	XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, XK_Control_L), True,
			  CurrentTime);
	XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, XK_Insert), True,
			  CurrentTime);

	XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, XK_Control_L), False,
			  CurrentTime);
	XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, XK_Insert), False,
			  CurrentTime);
	XSync(dpy, False);

	system("xclip -o|xclip -selection CLIPBOARD");
}

void scroll(int direction)
{
	int btn = 0;

	switch (direction) {
	case SCROLL_UP:
		btn = 4;
		break;
	case SCROLL_DOWN:
		btn = 5;
		break;
	case SCROLL_RIGHT:
		btn = 7;
		break;
	case SCROLL_LEFT:
		btn = 6;
		break;
	}

	XTestFakeButtonEvent(dpy, btn, True, CurrentTime);
	XTestFakeButtonEvent(dpy, btn, False, CurrentTime);
}

struct pixmap *create_pixmap(const char *color, int w, int h)
{
	uint8_t r, g, b, a;

	hex_to_rgba(color, &r, &g, &b, &a);
	struct pixmap *p = malloc(sizeof(struct pixmap));

	Pixmap pm = XCreatePixmap(dpy, DefaultRootWindow(dpy), w, h,
				  DefaultDepth(dpy, DefaultScreen(dpy)));

	GC gc =
	    XCreateGC(dpy, DefaultRootWindow(dpy), GCForeground | GCFillStyle,
		      &(XGCValues){
			  .foreground = xcolor(r, g, b),
			  .fill_style = FillSolid,
		      });

	XFillRectangle(dpy, pm, gc, 0, 0, w, h);

	p = malloc(sizeof(struct pixmap));

	p->map = pm;
	p->gc = gc;
	p->w = w;
	p->h = h;

	return p;
}

Window create_window(const char *color, int x, int y, int w, int h)
{
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
	uint8_t a = 0;

	XClassHint *hint;

	hex_to_rgba(color, &r, &g, &b, &a);

	w = w ? w : 1;
	h = h ? h : 1;

	Window win = XCreateWindow(
	    dpy, DefaultRootWindow(dpy), x, y, w, h, 0,
	    DefaultDepth(dpy, DefaultScreen(dpy)), InputOutput,
	    DefaultVisual(dpy, DefaultScreen(dpy)),
	    CWOverrideRedirect | CWBackPixel | CWBackingStore | CWBackingPixel,
	    &(XSetWindowAttributes){
		.backing_pixel = xcolor(r, g, b),
		.background_pixel = xcolor(r, g, b),
		.backing_store = Always,
		.override_redirect = 1,
	    });

	set_opacity(dpy, win, a);
	disable_compton_shadow(dpy, win);
	hint = XAllocClassHint();
	hint->res_name = "warpd";
	hint->res_class = "warpd";

	XSetClassHint(dpy, win, hint);
	XFree(hint);

	return win;
}

void mouse_up(int btn)
{
	XTestFakeButtonEvent(dpy, btn, False, CurrentTime);
	XSync(dpy, False);
}

void mouse_down(int btn)
{
	XTestFakeButtonEvent(dpy, btn, True, CurrentTime);
	XSync(dpy, False);
}

void mouse_click(int btn)
{
	XTestFakeButtonEvent(dpy, btn, True, CurrentTime);
	XTestFakeButtonEvent(dpy, btn, False, CurrentTime);

	XSync(dpy, False);
}

void mouse_move(int x, int y)
{
	XTestFakeMotionEvent(dpy, DefaultScreen(dpy), x, y, 0);
	XSync(dpy, False);
}

void mouse_get_position(int *x, int *y)
{
	Window	     chld, root;
	int	     _;
	unsigned int _u;

	/* Obtain absolute pointer coordinates */
	XQueryPointer(dpy, DefaultRootWindow(dpy), &root, &chld, x, y, &_, &_,
		      &_u);
}

void mouse_hide()
{
	if (!mouse_visible)
		return;

	XFixesHideCursor(dpy, DefaultRootWindow(dpy));
	XSync(dpy, False);
	mouse_visible = 0;
}

void mouse_show()
{
	if (mouse_visible)
		return;

	XFixesShowCursor(dpy, DefaultRootWindow(dpy));
	XSync(dpy, False);
	mouse_visible = 1;
}

void start_main_loop(void (*init)(void))
{
	dpy = XOpenDisplay(NULL);
	init();
}

void platform_commit() {}
