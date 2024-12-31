/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include "X.h"

static int nr_grabbed_device_ids = 0;
static int grabbed_device_ids[64];


uint8_t x_active_mods = 0;

/* clear the X keyboard state. */
static void reset_keyboard()
{
	size_t i;
	char keymap[32];

	XQueryKeymap(dpy, keymap);

	for (i = 0; i < 256; i++) {
		if (0x01 & keymap[i / 8] >> (i % 8))
			XTestFakeKeyEvent(dpy, i, 0, CurrentTime);
	}

	XSync(dpy, False);
}

static void grab(int device_id)
{
	int rc;

	XIEventMask mask;

	mask.deviceid = XIAllDevices;
	mask.mask_len = XIMaskLen(XI_LASTEVENT);
	mask.mask = calloc(mask.mask_len, sizeof(char));

	XISetMask(mask.mask, XI_KeyPress);
	XISetMask(mask.mask, XI_KeyRelease);

	if ((rc = XIGrabDevice(dpy, device_id, DefaultRootWindow(dpy),
			       CurrentTime, None, GrabModeAsync, GrabModeAsync,
			       False, &mask))) {
		int n;

		XIDeviceInfo *info = XIQueryDevice(dpy, device_id, &n);
		fprintf(stderr, "FATAL: Failed to grab keyboard %s: %d\n",
			info->name, rc);
		exit(-1);
	}

	XSync(dpy, False);
}

/* timeout in ms. */
static XEvent *get_next_xev(int timeout)
{
	static int xfd = 0;
	static XEvent ev;
	char buf[1024];

	if (XPending(dpy)) {
		XNextEvent(dpy, &ev);
		return &ev;
	}

	fd_set fds;

	if (!xfd)
		xfd = XConnectionNumber(dpy);

	FD_ZERO(&fds);
	FD_SET(xfd, &fds);

	select(xfd + 1, &fds, NULL, NULL,
	       timeout ? &(struct timeval){0, timeout * 1000} : NULL);

	if (XPending(dpy)) {
		XNextEvent(dpy, &ev);
		return &ev;
	} else
		return NULL;
}

/* returns a key code or 0 on failure. */
static uint8_t process_xinput_event(XEvent *ev, int *state, int *mods)
{
	XGenericEventCookie *cookie = &ev->xcookie;

	static int xiop = 0;
	if (!xiop) {
		int ev, err;

		if (!XQueryExtension(dpy, "XInputExtension", &xiop, &ev,
				     &err)) {
			fprintf(stderr,
				"FATAL: X Input extension not available.\n");
			exit(-1);
		}
	}

	/* not a xinput event.. */
	if (cookie->type != GenericEvent || cookie->extension != xiop ||
	    !XGetEventData(dpy, cookie))
		return 0;

	switch (cookie->evtype) {
		uint16_t code;
		XIDeviceEvent *dev;

	case XI_KeyPress:
		dev = (XIDeviceEvent *)(cookie->data);
		code = (uint8_t)dev->detail;

		*state = (dev->flags & XIKeyRepeat) ? 2 : 1;
		*mods = dev->mods.effective;

		XFreeEventData(dpy, cookie);

		return code;
	case XI_KeyRelease:
		dev = (XIDeviceEvent *)(cookie->data);
		code = (uint8_t)dev->detail;

		*state = 0;
		*mods = dev->mods.effective;

		XFreeEventData(dpy, cookie);

		return code;
	}

	fprintf(stderr, "FATAL: Unrecognized xinput event\n");
	exit(-1);
}

static const char *xerr_key = NULL;
static int input_xerr(Display *dpy, XErrorEvent *ev)
{
	fprintf(stderr,
		"ERROR: Failed to grab %s (ensure it isn't mapped by another application)\n",
		xerr_key);
	return 0;
}

static const char *input_tostr(struct input_event *ev)
{
	static char s[64];
	const char *name = x_input_lookup_name(ev->code, ev->mods & PLATFORM_MOD_SHIFT ? 1 : 0);
	int n = 0;

	if (!ev)
		return "NULL";

	if (ev->mods & PLATFORM_MOD_CONTROL) {
		s[n++] = 'C';
		s[n++] = '-';
	}

	if (ev->mods & PLATFORM_MOD_ALT) {
		s[n++] = 'A';
		s[n++] = '-';
	}

	if (ev->mods & PLATFORM_MOD_META) {
		s[n++] = 'M';
		s[n++] = '-';
	}

	strcpy(s + n, name ? name : "UNDEFINED");

	return s;
}

static void xgrab_key(uint8_t code, uint8_t mods, int grab)
{
	XSetErrorHandler(input_xerr);
	int xmods = 0;

	if (!code)
		return;

	if (mods & PLATFORM_MOD_CONTROL)
		xmods |= ControlMask;
	if (mods & PLATFORM_MOD_SHIFT)
		xmods |= ShiftMask;
	if (mods & PLATFORM_MOD_META)
		xmods |= Mod4Mask;
	if (mods & PLATFORM_MOD_ALT)
		xmods |= Mod1Mask;

	xerr_key = input_tostr(&(struct input_event){code, mods, 0});

	if (grab) {
		XGrabKey(dpy, code, xmods, DefaultRootWindow(dpy), False,
			 GrabModeAsync, GrabModeAsync);

		XGrabKey(dpy, code, xmods | Mod2Mask, /* numlock */
			 DefaultRootWindow(dpy), False, GrabModeAsync, GrabModeAsync);
	} else {
		XUngrabKey(dpy, code, xmods, DefaultRootWindow(dpy));
		XUngrabKey(dpy, code, xmods | Mod2Mask, DefaultRootWindow(dpy));
	}

	XSync(dpy, False);

	XSetErrorHandler(NULL);
}

void x_input_grab_keyboard()
{
	int i, n;
	XIDeviceInfo *devices;

	if (nr_grabbed_device_ids != 0)
		return;

	devices = XIQueryDevice(dpy, XIAllDevices, &n);

	for (i = 0; i < n; i++) {
		if (devices[i].use == XISlaveKeyboard ||
		    devices[i].use == XIFloatingSlave) {
			if (!strstr(devices[i].name, "XTEST") &&
			    devices[i].enabled) {
				int id = devices[i].deviceid;

				grab(id);
				grabbed_device_ids[nr_grabbed_device_ids++] =
				    id;
			}
		}
	}

	/* send a key up event for any depressed keys to avoid infinite repeat. */
	reset_keyboard();
	XIFreeDeviceInfo(devices);

	x_active_mods = 0;
	XSync(dpy, False);
}

void x_input_ungrab_keyboard()
{
	int i;

	if (!nr_grabbed_device_ids)
		return;

	for (i = 0; i < nr_grabbed_device_ids; i++) {
		int n;
		XIDeviceInfo *info =
		    XIQueryDevice(dpy, grabbed_device_ids[i], &n);

		assert(n == 1);

		/*
		 * NOTE: Attempting to ungrab a disabled xinput device
		 * causes X to crash.
		 *
		 * (see
		 * https://gitlab.freedesktop.org/xorg/lib/libxi/-/issues/11).
		 *
		 * This generally shouldn't happen unless the user
		 * switches virtual terminals while warpd is running. We
		 * used to explicitly check for this and perform weird
		 * hacks to mitigate against it, but now we only grab
		 * the keyboard when the program is in one if its
		 * active modes which reduces the likelihood
		 * sufficiently to not to warrant the additional
		 * complexity.
		 */

		assert(info->enabled);
		XIUngrabDevice(dpy, grabbed_device_ids[i], CurrentTime);
	}

	nr_grabbed_device_ids = 0;
	XSync(dpy, False);
}

uint8_t xmods_to_mods(int xmods)
{
	uint8_t mods = 0;

	if (xmods & ShiftMask)
		mods |= PLATFORM_MOD_SHIFT;
	if (xmods & ControlMask)
		mods |= PLATFORM_MOD_CONTROL;
	if (xmods & Mod1Mask)
		mods |= PLATFORM_MOD_ALT;
	if (xmods & Mod4Mask)
		mods |= PLATFORM_MOD_META;

	return mods;
}

uint8_t get_code_modifier(uint8_t code)
{
	KeySym sym = XKeycodeToKeysym(dpy, code, 0);
	switch (sym) {
	case XK_Control_L:
	case XK_Control_R:
		return PLATFORM_MOD_CONTROL;
	case XK_Meta_L:
	case XK_Meta_R:
		return PLATFORM_MOD_META;
	case XK_Alt_L:
	case XK_Alt_R:
		return PLATFORM_MOD_ALT;
	case XK_Shift_L:
	case XK_Shift_R:
		return PLATFORM_MOD_SHIFT;
	default:
		return 0;
	}
}

/* returns 0 on timeout. */
struct input_event *x_input_next_event(int timeout)
{
	static struct input_event ev;

	struct timeval start, end;
	gettimeofday(&start, NULL);
	int elapsed = 0;

	while (1) {
		int state;
		uint8_t code;
		XEvent *xev;

		xev = get_next_xev(timeout - elapsed);

		if (xev) {
			int xmods;
			code = process_xinput_event(xev, &state, &xmods);
			if (code && state != 2) {
				ev.pressed = state;
				ev.code = code;
				ev.mods = xmods_to_mods(xmods);

				if (state)
					x_active_mods |= get_code_modifier(code);
				else
					x_active_mods &= ~get_code_modifier(code);

				return &ev;
			}
		}

		if (timeout) {
			gettimeofday(&end, NULL);
			elapsed = (end.tv_sec - start.tv_sec) * 1000 +
				  (end.tv_usec - start.tv_usec) / 1000;

			if (elapsed >= timeout)
				return NULL;
		}
	}
}

struct input_event *x_input_wait(struct input_event *events, size_t sz)
{
	size_t i;
	static struct input_event ev;
	struct input_event *ret = NULL;

	for (i = 0; i < sz; i++) {
		struct input_event *ev = &events[i];
		xgrab_key(ev->code, ev->mods, 1);
	}

	while (1) {
		XEvent *xev = get_next_xev(100);

		if (xev && (xev->type == KeyPress || xev->type == KeyRelease)) {
			ev.code = (uint8_t)xev->xkey.keycode;
			ev.mods = xmods_to_mods(xev->xkey.state);
			ev.pressed = xev->type == KeyPress;

			x_input_grab_keyboard();

			ret = &ev;
			goto exit;
		} else {
			size_t i;
			for (i = 0; i < nr_monitored_files; i++) {
				long mtime = x_get_mtime(monitored_files[i].path);
				if (mtime != monitored_files[i].mtime) {
					monitored_files[i].mtime = mtime;
					goto exit;
				}
			}
		}
	}

exit:
	for (i = 0; i < sz; i++) {
		struct input_event *ev = &events[i];
		xgrab_key(ev->code, ev->mods, 0);
	}

	return ret;
}

/* Normalize keynames for non API code. */
struct {
	const char *name;
	const char *xname;
} normalization_map[] = {
	{"esc", "Escape"},
	{",", "comma"},
	{".", "period"},
	{"-", "minus"},
	{"/", "slash"},
	{";", "semicolon"},
	{"[", "bracketleft"},
	{"]", "bracketright"},
	{"'", "apostrophe"},
	{"$", "dollar"},
	{"backspace", "BackSpace"},
};

uint8_t x_input_lookup_code(const char *name, int *shifted)
{
	uint8_t code = 0;
	size_t i;

	for (i = 0; i < sizeof normalization_map / sizeof normalization_map[0]; i++)
		if (!strcmp(normalization_map[i].name, name))
			name = normalization_map[i].xname;

	KeySym sym = XStringToKeysym(name);

	if (!sym)
		return 0;
	
	code = XKeysymToKeycode(dpy, sym);

	if (XKeycodeToKeysym(dpy, code, 0) != sym)
		*shifted = 1;
	else
		*shifted = 0;


	return code;
}

const char *x_input_lookup_name(uint8_t code, int shifted)
{
	size_t i;
	const char *name;
	KeySym sym = XKeycodeToKeysym(dpy, code, shifted);
	if (!sym)
		return NULL;

	name = XKeysymToString(sym);

	for (i = 0; i < sizeof normalization_map / sizeof normalization_map[0]; i++)
		if (!strcmp(normalization_map[i].xname, name))
			name = normalization_map[i].name;

	return name;
}
