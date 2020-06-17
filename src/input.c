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
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <ctype.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/XInput2.h>
#include <assert.h>

#include "dbg.h"
#include "input.h"

/*
 * Use Xinput instead of standard X grabs to avoid interference with X toolkits
 * and window managers. This isn't a silver bullet since such grabs are still
 * mutually exclusive and potentially present the same problems, but do so at a
 * lower level and appear to be less commonly used. Note that master device
 * grabs and normal X grabs are still mutually exclusive which is why we
 * attempt to wrest control of the physical devices away from their
 * corresponding virtual devices (which can still fail if they are individually
 * grabbed, though this is unlikely).
 *
 * NOTE: These functions discard non-input X events. Ideally we should
 * implement a custom event loop, but the current API is cleaner and sufficient
 * for the intended use case.

*/

static Display *dpy;
static size_t nkbds;
static int kbds[256];

static int active_mods = 0;

static struct {
	KeySym sym;
	uint16_t mask;
	KeyCode code;
} mods[] = {
	{ XK_Shift_L, ShiftMask, 0},
	{ XK_Shift_R, ShiftMask, 0},
	{ XK_Control_L, ControlMask, 0},
	{ XK_Control_R, ControlMask, 0},
	{ XK_Super_R, Mod4Mask, 0},
	{ XK_Super_L, Mod4Mask, 0},
	{ XK_Alt_L, Mod1Mask, 0},
	{ XK_Alt_R, Mod1Mask, 0},
};

static void rescan_devices()
{
	int i, n;

	XIDeviceInfo* devs = XIQueryDevice(dpy, XIAllDevices, &n);

	nkbds = 0;

	for (i = 0; i < n; i++) {
		if (devs[i].use == XISlaveKeyboard ||
		    devs[i].use == XIFloatingSlave) {
			dbg("detected keyboard: %s (%d)", devs[i].name, devs[i].deviceid);
			kbds[nkbds++] = devs[i].deviceid;
		}
	}

	XIFreeDeviceInfo(devs);
}

static uint16_t get_mod_mask(uint16_t code)
{
	size_t i;

	for (i = 0; i < sizeof mods / sizeof mods[0]; i++) {
		if(mods[i].code == code)
			return (mods[i].mask << 8);
	}

	return 0;
}

static void clear_keys()
{
	size_t i;
	char keymap[32];

	XQueryKeymap(dpy, keymap);

	for (i = 0; i < sizeof keymap; i++) {
		int j;
		for (j = 0;j<8;j++) {
			if(0x01 & (keymap[i] >> j)) {
				KeyCode code = 8*i+j;
				XTestFakeKeyEvent(dpy, code, 0, CurrentTime);
			}
		}
	}

	XFlush(dpy);
}

uint16_t input_wait_for_key(uint16_t *keys, size_t n)
{
	size_t i;

	for (i = 0; i < n; i++) {
		// Ensure X doesn't process the key without modifiers. We don't
		// actually care about this event.
		XGrabKey(dpy,
			 keys[i] & 0xFF,
			 keys[i] >> 8,
			 DefaultRootWindow(dpy),
			 False,
			 0, 0);

		XGrabKey(dpy,
			 keys[i] & 0xFF,
			 (keys[i] >> 8) | Mod2Mask,
			 DefaultRootWindow(dpy),
			 False,
			 0, 0);
	}

	while(1) {
		uint16_t key = input_next_key(0, 0);
		XUngrabKeyboard(dpy, CurrentTime);

		for (i = 0; i < n; i++) {
			if(key == keys[i])
				return key;
		}
	}

}

void input_grab_keyboard()
{
	size_t i;
	XIEventMask mask;

	dbg("Grabbing keyboard");

	mask.deviceid = XIAllDevices;
	mask.mask_len = XIMaskLen(XI_LASTEVENT);
	mask.mask = calloc(mask.mask_len, sizeof(char));
	XISetMask(mask.mask, XI_KeyPress);
	XISetMask(mask.mask, XI_KeyRelease);

	clear_keys();
	for (i = 0; i < nkbds; i++) {
		if(XIGrabDevice(dpy, kbds[i],
				DefaultRootWindow(dpy),
				CurrentTime,
				None,
				GrabModeAsync,
				GrabModeAsync,
				False, &mask)) {
			fprintf(stderr, "FATAL: Failed to grab keyboard\n");
			exit(-1);
		}
	}

	XSync(dpy, False);
	dbg("Done");
}

const char* input_keyseq_to_string(uint16_t seq)
{
	int i = 0;
	static char s[256];
	uint8_t mods = seq >> 8;
	uint8_t code = seq & 0xFF;
	if(seq == 0) return "UNDEFINED";

	if(mods & Mod1Mask) {
		s[i++] = 'A';
		s[i++] = '-';
	}
	if(mods & Mod4Mask) {
		s[i++] = 'M';
		s[i++] = '-';
	}
	if(mods & ShiftMask) {
		s[i++] = 'S';
		s[i++] = '-';
	}
	if(mods & ControlMask) {
		s[i++] = 'C';
		s[i++] = '-';
	}

	strcpy(s + i, XKeysymToString(XKeycodeToKeysym(dpy, code, 0)));

	return s;
}

uint16_t input_parse_keyseq(const char* key) 
{
	if(!key || key[0] == 0) return 0;

	uint8_t code = 0;
	uint8_t mods = 0;

	while(key[1] == '-') {
		switch(key[0]) {
		case 'A':
			mods |= Mod1Mask;
			break;
		case 'M':
			mods |= Mod4Mask;
			break;
		case 'S':
			mods |= ShiftMask;
			break;
		case 'C':
			mods |= ControlMask;
			break;
		default:
			fprintf(stderr, "%s is not a valid key\n", key);
			exit(1);
		}

		key += 2;
	}

	if(key[0]) {
		KeySym sym = XStringToKeysym(key);

		if(sym == NoSymbol) {
			fprintf(stderr, "Could not find keysym for %s\n", key);
			exit(1);
		}

		if(key[1] == '\0' && isupper(key[0]))
			mods |= ShiftMask;

		code = XKeysymToKeycode(dpy, sym);
	}

	return (mods << 8) | code;
}

void init_input(Display *_dpy)
{
	size_t i;
	dpy = _dpy;

	for (i = 0; i < sizeof mods / sizeof mods[0]; i++)
		mods[i].code = XKeysymToKeycode(dpy, mods[i].sym);

	XIEventMask evmask;

	evmask.deviceid = XIAllDevices;
	evmask.mask_len = XIMaskLen(XI_LASTEVENT);
	evmask.mask = calloc(evmask.mask_len, sizeof(char));

	XISetMask(evmask.mask, XI_HierarchyChanged);
	XISetMask(evmask.mask, XI_KeyPress);
	XISetMask(evmask.mask, XI_KeyRelease);
	XISelectEvents(dpy, DefaultRootWindow(dpy), &evmask, 1);

	free(evmask.mask);
	XFlush(dpy);

	rescan_devices();
}

void input_ungrab_keyboard()
{
	size_t i;

	dbg("Ungrabbing keyboard");
	for (i = 0; i < nkbds; i++) {
		dbg("Ungrabbing %d", kbds[i]);
		XIUngrabDevice(dpy, kbds[i], CurrentTime);
		dbg("Done");
	}

	XFlush(dpy);
	clear_keys();
}

void input_click(int btn) 
{
	uint16_t mods = (active_mods >> 8);
	input_ungrab_keyboard();
	if(Mod1Mask & mods)
		XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, XK_Alt_L), 1, CurrentTime);
	if(ShiftMask & mods)
		XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, XK_Shift_L), 1, CurrentTime);
	if(Mod4Mask & mods)
		XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, XK_Super_L), 1, CurrentTime);
	if(ControlMask & mods)
		XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, XK_Control_L), 1, CurrentTime);

	XTestFakeButtonEvent(dpy, btn, True, CurrentTime);
	XTestFakeButtonEvent(dpy, btn, False, CurrentTime);

	if(Mod1Mask & mods)
		XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, XK_Alt_L), 0, CurrentTime);
	if(ShiftMask & mods)
		XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, XK_Shift_L), 0, CurrentTime);
	if(Mod4Mask & mods)
		XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, XK_Super_L), 0, CurrentTime);
	if(ControlMask & mods)
		XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, XK_Control_L), 0, CurrentTime);

	input_grab_keyboard();
	XFlush(dpy);
}

static int process_xev(XEvent *ev, uint16_t *keyseq)
{
	XGenericEventCookie *cookie = &ev->xcookie;

	static int xiop = 0;
	if(!xiop) {
		int ev, err;

		if (!XQueryExtension(dpy, "XInputExtension", &xiop, &ev, &err)) {
			fprintf(stderr, "FATAL: X Input extension not available.\n");
			exit(-1);
		}
	}

	if (cookie->type != GenericEvent ||
	    cookie->extension != xiop ||
	    !XGetEventData(dpy, cookie))
		return EV_XEV;

	switch(cookie->evtype) {
		XIDeviceEvent *dev;
		XIHierarchyEvent *hev;
		uint16_t mask;

	case XI_HierarchyChanged:
		hev = (XIHierarchyEvent*)cookie->data;

		if(hev->flags & (XIDeviceEnabled | XIDeviceDisabled))
			rescan_devices();

		XFreeEventData(dpy, cookie);
		return EV_DEVICE_CHANGE;
	case XI_KeyPress:
		dev = (XIDeviceEvent*)(cookie->data);
		mask = get_mod_mask(dev->detail); 

		if(mask)
			active_mods |= mask;
		else {
			int type;
			if(keyseq)
				*keyseq = active_mods | dev->detail;

			type = (dev->flags & XIKeyRepeat) ? EV_KEYREPEAT : EV_KEYPRESS;
			XFreeEventData(dpy, cookie);
			return type;
		}

		XFreeEventData(dpy, cookie);
		return EV_MOD;
	case XI_KeyRelease:
		dev = (XIDeviceEvent*)(cookie->data);

		mask = get_mod_mask(dev->detail);
		if(mask) {
			active_mods &= ~mask;
			return EV_MOD;
		}

		if(keyseq)
			*keyseq = active_mods | dev->detail;

		XFreeEventData(dpy, cookie);
		return EV_KEYRELEASE;
	}

	fprintf(stderr, "FATAL: Unrecognized xinput event\n");
	exit(-1);
}

static XEvent *get_xev(int timeout)
{
	static int xfd = 0;
	static XEvent ev;
	fd_set fds;

	if(!xfd) xfd = XConnectionNumber(dpy);

	if(XPending(dpy)) {
		XNextEvent(dpy, &ev);
		return &ev;
	}

	FD_ZERO(&fds);
	FD_SET(xfd, &fds);
	select(xfd+1,
	       &fds,
	       NULL,
	       NULL,
	       timeout ? &(struct timeval){0, timeout*1000} : NULL);

	if(XPending(dpy)) {
		XNextEvent(dpy, &ev);
		return &ev;
	} else
		return NULL;

}

int input_next_ev(int timeout, uint16_t *keyseq)
{
	int type;
	XEvent *ev = get_xev(timeout);
	if(!ev) 
		return EV_TIMEOUT;

	type = process_xev(ev, keyseq);
	if(type == EV_MOD)
		return input_next_ev(timeout, keyseq);
	else
		return type;
}

uint16_t input_next_key(int timeout, int include_repeats)
{
	struct timeval start, end;
	gettimeofday(&start, NULL);

	int elapsed = 0;
	while(1) {
		uint16_t key;
		XEvent *ev;

		ev = get_xev(timeout - elapsed);
		if(!ev) return TIMEOUT_KEYSEQ;

		switch(process_xev(ev, &key)) {
		case EV_KEYREPEAT:
			if(include_repeats)
				return key;
			break;
		case EV_KEYPRESS:
			return key;
			break;
		}

		if(timeout) {
			gettimeofday(&end, NULL);
			elapsed = (end.tv_sec-start.tv_sec)*1000 + (end.tv_usec-start.tv_usec)/1000;
			if(elapsed >= timeout)
				break;
		}
	}

	return TIMEOUT_KEYSEQ;
}

void input_get_cursor_position(int *x, int *y)
{
	Window chld, root;
	int _;
	unsigned int _u;

	/* Obtain absolute pointer coordinates */
	XQueryPointer(dpy, DefaultRootWindow(dpy), &root, &chld, x, y, &_, &_, &_u);
}
