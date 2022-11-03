/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include "macos.h"

static int grabbed = 0;
static long grabbed_time;

static int input_fds[2];
static struct input_event *grabbed_keys;
static size_t grabbed_keys_sz = 0;

static uint8_t passthrough_keys[256] = {0};

static CFMachPortRef tap;

uint8_t active_mods = 0;
pthread_mutex_t keymap_mtx = PTHREAD_MUTEX_INITIALIZER;

static struct {
	char *name;
	char *shifted_name;
} keymap[256] = { 0 };

struct mod {
	uint8_t mask;
	uint8_t code1;
	uint8_t code2;
} modifiers[] = {
    {MOD_CONTROL, 60, 63},
    {MOD_SHIFT, 57, 61},
    {MOD_META, 55, 56},
    {MOD_ALT, 59, 62},
};

static long get_time_ms()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);

	return ts.tv_nsec / 1E6 + ts.tv_sec * 1E3;
}

static void write_message(int fd, void *msg, ssize_t sz)
{
	assert(write(fd, msg, sz) == sz);
}

/* Returns -1 if the timeout expires before a message is available. */
static int read_message(int fd, void *msg, ssize_t sz, int timeout)
{
	fd_set fds;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	select(fd + 1, &fds, NULL, NULL,
	       timeout ? &(struct timeval){.tv_usec = timeout * 1E3} : NULL);

	/* timeout */
	if (!FD_ISSET(fd, &fds))
		return -1;

	assert(read(fd, msg, sz) == sz);

	return 0;
}

static CGEventRef eventTapCallback(CGEventTapProxy proxy, CGEventType type,
				   CGEventRef event, void *context)
{
	size_t i;
	int is_key_event = 0;

	uint8_t code = 0;
	uint8_t pressed = 0;
	uint8_t mods = 0;

	static uint8_t keymods[256] = {0}; /* Mods active at key down time. */
	static long pressed_timestamps[256];

	/* macOS will timeout the event tap, so we have to re-enable it :/ */
	if (type == kCGEventTapDisabledByTimeout) {
		CGEventTapEnable(tap, true);
		return event;
	}

	/* If only apple designed its system APIs like its macbooks... */
	switch (type) {
		NSEvent *nsev;
		CGEventFlags flags;

	case NX_SYSDEFINED: /* system codes (e.g brightness) */
		nsev = [NSEvent eventWithCGEvent:event];

		code = (nsev.data1 >> 16) + 220;
		pressed = !(nsev.data1 & 0x100);

		/*
		 * Pass other system events through, things like sticky keys
		 * rely on NX_SYSDEFINED events for visual notifications.
		 */
		if (nsev.subtype == NX_SUBTYPE_AUX_CONTROL_BUTTONS)
			is_key_event = 1;

		break;
	case kCGEventFlagsChanged: /* modifier codes */
		code = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode) + 1;
		flags = CGEventGetFlags(event);
		pressed = 0;

		switch (code) {
			case 57: case 61: pressed = !!(flags & kCGEventFlagMaskShift); break;
			case 59: case 62: pressed = !!(flags & kCGEventFlagMaskAlternate); break;
			case 55: case 56: pressed = !!(flags & kCGEventFlagMaskCommand); break;
			case 60: case 63: pressed = !!(flags & kCGEventFlagMaskControl); break;
		}

		is_key_event = 1;
		break;
	case kCGEventKeyDown:
	case kCGEventKeyUp:
		/* Skip repeat events */
		if (CGEventGetIntegerValueField(event, kCGKeyboardEventAutorepeat)) {
			if (grabbed)
				return nil;
			else
				return event;
		}

		/*
		 * We shift codes up by 1 so 0 is not a valid code. This is
		 * accounted for in the name table.
		 */
		code = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode) + 1;
		pressed = type == kCGEventKeyDown;

		is_key_event = 1;
		break;
	default:
		break;
	}

	if (!is_key_event)
		return event;

	if (pressed == 1)
		pressed_timestamps[code] = get_time_ms();

	if (passthrough_keys[code]) {
		passthrough_keys[code]--;
		return event;
	}

	/* Compute the active mod set. */
	for (i = 0; i < sizeof modifiers / sizeof modifiers[0]; i++) {
		struct mod *mod = &modifiers[i];

		if (code == mod->code1 || code == mod->code2) {
			if (pressed)
				active_mods |= mod->mask;
			else
				active_mods &= ~mod->mask;
		}
	}

	/* Ensure mods are consistent across keydown/up events. */
	if (pressed == 0) {
		mods = keymods[code];
	} else if (pressed == 1) {
		mods = active_mods;
		keymods[code] = mods;
	}

	struct input_event ev;

	ev.code = code;
	ev.pressed = pressed;
	ev.mods = mods;

	write_message(input_fds[1], &ev, sizeof ev);

	for (i = 0; i < grabbed_keys_sz; i++)
		if (grabbed_keys[i].code == code &&
		    grabbed_keys[i].mods == active_mods) {
			grabbed = 1;
			grabbed_time = get_time_ms();
			return nil;
		}

	if (grabbed) {
		/* If the keydown occurred before the grab, allow the keyup to pass through. */
		if (pressed || pressed_timestamps[code] > grabbed_time) {
			return nil;
		}
	}
	return event;
}

/*
 * TODO: make sure names are consistent with the linux map + account
 * for OS keymap.
 */
const char *code_to_string(uint8_t code, int shifted)
{
	static char name[256];
	pthread_mutex_lock(&keymap_mtx);
	strcpy(name, shifted ? keymap[code].shifted_name : keymap[code].name);
	pthread_mutex_unlock(&keymap_mtx);

	return name;
}

const char *osx_input_lookup_name(uint8_t code, int shifted)
{
	return code_to_string(code, shifted);
}

uint8_t osx_input_lookup_code(const char *name, int *shifted)
{
	size_t i;
	pthread_mutex_lock(&keymap_mtx);

	/*
	 * Horribly inefficient.
	 *
	 * TODO: Figure out the right Carbon incantation for reverse
	 * name lookups.
	 */
	for (i = 0; i < 256; i++) {
		if (keymap[i].name && !strcmp(name, keymap[i].name)) {
			*shifted = 0;
			pthread_mutex_unlock(&keymap_mtx);
			return i;
		} else if (keymap[i].shifted_name && !strcmp(name, keymap[i].shifted_name)) {
			*shifted = 1;
			pthread_mutex_unlock(&keymap_mtx);
			return i;
		}
	}

	pthread_mutex_unlock(&keymap_mtx);
	return 0;
}

static void _send_key(uint8_t code, int pressed)
{
	static int command_down = 0;

	/* left/right command keys */
	if (code == 56 || code == 55)
		command_down += pressed ? 1 : -1;

	/* events should bypass any active grabs */
	passthrough_keys[code]++;
	CGEventRef ev = CGEventCreateKeyboardEvent(NULL, code - 1, pressed);

	/* quartz inspects the event flags instead of maintaining its own state */
	if (command_down)
		CGEventSetFlags(ev, kCGEventFlagMaskCommand);

	CGEventPost(kCGHIDEventTap, ev);
	CFRelease(ev);
}

void send_key(uint8_t code, int pressed)
{
	dispatch_sync(dispatch_get_main_queue(), ^{
		_send_key(code, pressed);
	});
}

void osx_input_ungrab_keyboard()
{
	dispatch_sync(dispatch_get_main_queue(), ^{
		grabbed = 0;
	});
}

void osx_input_grab_keyboard()
{
	if (grabbed)
		return;

	dispatch_sync(dispatch_get_main_queue(), ^{
		grabbed = 1;
		grabbed_time = get_time_ms();
	});
}

struct input_event *osx_input_next_event(int timeout)
{
	static struct input_event ev;

	if (read_message(input_fds[0], &ev, sizeof ev, timeout) < 0)
		return 0;

	return &ev;
}

struct input_event *osx_input_wait(struct input_event *keys, size_t sz)
{
	grabbed_keys = keys;
	grabbed_keys_sz = sz;

	while (1) {
		size_t i;
		struct input_event *ev = osx_input_next_event(0);

		for (i = 0; i < sz; i++)
			if (ev->pressed && keys[i].code == ev->code &&
			    keys[i].mods == ev->mods) {
				grabbed_keys = NULL;
				grabbed_keys_sz = 0;

				return ev;
			}
	}
}

static void update_keymap()
{
	pthread_mutex_lock(&keymap_mtx);

	int code;
	UInt32 deadkeystate = 0;
	UniChar chars[4];
	UniCharCount len;
	CFStringRef str;
	TISInputSourceRef kbd = TISCopyCurrentKeyboardLayoutInputSource();

	assert(kbd);

	for (code = 1; code < 256; code++) {
		if (!keymap[code].name) {
			keymap[code].name = malloc(32);
			keymap[code].shifted_name = malloc(32);
		}
		/* Blech */
		CFDataRef layout_data = TISGetInputSourceProperty(kbd, kTISPropertyUnicodeKeyLayoutData);
		const UCKeyboardLayout *layout = (const UCKeyboardLayout *)CFDataGetBytePtr(layout_data);

		UCKeyTranslate(layout, code-1, kUCKeyActionDisplay, 0, LMGetKbdType(),
			       kUCKeyTranslateNoDeadKeysBit, &deadkeystate,
			       sizeof(chars) / sizeof(chars[0]), &len, chars);

		str = CFStringCreateWithCharacters(kCFAllocatorDefault, chars, 1);
		CFStringGetCString(str, keymap[code].name, 32, kCFStringEncodingUTF8);
		CFRelease(str);

		UCKeyTranslate(layout, code-1, kUCKeyActionDisplay, 2, LMGetKbdType(),
			       kUCKeyTranslateNoDeadKeysBit, &deadkeystate,
			       sizeof(chars) / sizeof(chars[0]), &len, chars);

		str = CFStringCreateWithCharacters(kCFAllocatorDefault, chars, 1);
		CFStringGetCString(str, keymap[code].shifted_name, 32, kCFStringEncodingUTF8);
		CFRelease(str);

		if (!strcmp(keymap[code].name, "\033"))
			strcpy(keymap[code].name, "esc");
		if (!strcmp(keymap[code].name, ""))
			strcpy(keymap[code].name, "backspace");

		switch (code) {
			case 55: strcpy(keymap[code].name, "rightmeta"); break;
			case 56: strcpy(keymap[code].name, "leftmeta"); break;
			case 57: strcpy(keymap[code].name, "leftshift"); break;
			case 58: strcpy(keymap[code].name, "capslock"); break;
			case 59: strcpy(keymap[code].name, "leftalt"); break;
			case 60: strcpy(keymap[code].name, "leftcontrol"); break;
			case 61: strcpy(keymap[code].name, "rightshift"); break;
			case 62: strcpy(keymap[code].name, "rightalt"); break;
			case 63: strcpy(keymap[code].name, "rightcontrol"); break;
		}

	}

	pthread_mutex_unlock(&keymap_mtx);
}

/* Called by the main thread to set up event stream. */
void macos_init_input()
{
	/* Request accessibility access if not present. */
	NSDictionary *options = @{(id)kAXTrustedCheckOptionPrompt : @YES};
	BOOL access = AXIsProcessTrustedWithOptions((CFDictionaryRef)options);

	if (!access) {
		printf("Waiting for accessibility permissions\n");
		tap = nil;
		while (!tap) {
			tap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, 0,
					     kCGEventMaskForAllEvents, eventTapCallback, NULL);
			usleep(100000);
		}
		printf("Accessibility permission granted, proceeding\n");
	} else {
		tap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, 0,
				     kCGEventMaskForAllEvents, eventTapCallback, NULL);
	}


	if (!tap) {
		fprintf(stderr,
			"Failed to create event tap, make sure warpd is "
			"whitelisted as an accessibility feature.\n");
		exit(-1);
	}

	CFRunLoopSourceRef runLoopSource =
	    CFMachPortCreateRunLoopSource(kCFAllocatorDefault, tap, 0);

	CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource,
			   kCFRunLoopCommonModes);


	CGEventTapEnable(tap, true);

	CFNotificationCenterAddObserver(
	    CFNotificationCenterGetLocalCenter(), NULL, update_keymap,
	    CFSTR("NSTextInputContextKeyboardSelectionDidChangeNotification"),
	    NULL, CFNotificationSuspensionBehaviorDeliverImmediately);

	update_keymap();

	if (pipe(input_fds) < 0) {
		perror("pipe");
		exit(-1);
	}
}
