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
	char name[32];
	char shifted_name[32];
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
const char *osx_input_lookup_name(uint8_t code, int shifted)
{
	static char name[256];

	pthread_mutex_lock(&keymap_mtx);
	strcpy(name, shifted ? keymap[code].shifted_name : keymap[code].name);
	pthread_mutex_unlock(&keymap_mtx);

	if (!name[0])
		return NULL;

	return name;
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
		if (!strcmp(name, keymap[i].name)) {
			*shifted = 0;
			pthread_mutex_unlock(&keymap_mtx);
			return i;
		} else if (!strcmp(name, keymap[i].shifted_name)) {
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
	static uint8_t valid_keycodes[256] = {
		[0x01] = 1, [0x02] = 1, [0x03] = 1, [0x04] = 1, [0x05] = 1, [0x06] = 1, [0x07] = 1, [0x08] = 1,
		[0x09] = 1, [0x0a] = 1, [0x0b] = 1, [0x0c] = 1, [0x0d] = 1, [0x0e] = 1, [0x0f] = 1, [0x10] = 1,
		[0x11] = 1, [0x12] = 1, [0x13] = 1, [0x14] = 1, [0x15] = 1, [0x16] = 1, [0x17] = 1, [0x18] = 1,
		[0x19] = 1, [0x1a] = 1, [0x1b] = 1, [0x1c] = 1, [0x1d] = 1, [0x1e] = 1, [0x1f] = 1, [0x20] = 1,
		[0x21] = 1, [0x22] = 1, [0x23] = 1, [0x24] = 1, [0x25] = 1, [0x26] = 1, [0x27] = 1, [0x28] = 1,
		[0x29] = 1, [0x2a] = 1, [0x2b] = 1, [0x2c] = 1, [0x2d] = 1, [0x2e] = 1, [0x2f] = 1, [0x30] = 1,
		[0x31] = 1, [0x32] = 1, [0x33] = 1, [0x34] = 1, [0x36] = 1, [0x37] = 1, [0x38] = 1, [0x39] = 1,
		[0x3a] = 1, [0x3b] = 1, [0x3c] = 1, [0x3d] = 1, [0x3e] = 1, [0x3f] = 1, [0x40] = 1, [0x41] = 1,
		[0x42] = 1, [0x44] = 1, [0x46] = 1, [0x48] = 1, [0x49] = 1, [0x4a] = 1, [0x4b] = 1, [0x4c] = 1,
		[0x4d] = 1, [0x4f] = 1, [0x50] = 1, [0x51] = 1, [0x52] = 1, [0x53] = 1, [0x54] = 1, [0x55] = 1,
		[0x56] = 1, [0x57] = 1, [0x58] = 1, [0x59] = 1, [0x5a] = 1, [0x5b] = 1, [0x5c] = 1, [0x5d] = 1,
		[0x5e] = 1, [0x5f] = 1, [0x60] = 1, [0x61] = 1, [0x62] = 1, [0x63] = 1, [0x64] = 1, [0x65] = 1,
		[0x66] = 1, [0x67] = 1, [0x68] = 1, [0x69] = 1, [0x6a] = 1, [0x6b] = 1, [0x6c] = 1, [0x6e] = 1,
		[0x6f] = 1, [0x70] = 1, [0x72] = 1, [0x73] = 1, [0x74] = 1, [0x75] = 1, [0x76] = 1, [0x77] = 1,
		[0x78] = 1, [0x79] = 1, [0x7a] = 1, [0x7b] = 1, [0x7c] = 1, [0x7d] = 1, [0x7e] = 1, [0x7f] = 1
	};

	pthread_mutex_lock(&keymap_mtx);

	int code;
	UInt32 deadkeystate = 0;
	UniChar chars[4];
	UniCharCount len;
	CFStringRef str;
	TISInputSourceRef kbd = TISCopyCurrentKeyboardLayoutInputSource();

	assert(kbd);

	for (code = 1; code < 256; code++) {
		if (!valid_keycodes[code]) {
			keymap[code].name[0] = 0;
			keymap[code].shifted_name[0] = 0;
			continue;
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

		//TODO: probably missing some keys...
#define fixup(keyname, newname) \
	if (!strcmp(keymap[code].name, keyname)) { \
		strcpy(keymap[code].name, newname); \
		strcpy(keymap[code].shifted_name, ""); \
	}
		fixup("\033", "esc");
		fixup("\x08", "backspace");
		fixup("\x7f", "delete");
#undef fixup

		if (keymap[code].name[0] < 31) { /* Exclude anything else with non printable characters (control codes) */
			strcpy(keymap[code].name, "");
			strcpy(keymap[code].shifted_name, "");
		}

		switch (code) {
#define set_name(keyname) \
	strcpy(keymap[code].name, keyname); \
	strcpy(keymap[code].shifted_name, ""); \
	break;
			case 55: set_name("rightmeta")
			case 56: set_name("leftmeta")
			case 57: set_name("leftshift")
			case 58: set_name("capslock")
			case 59: set_name("leftalt")
			case 60: set_name("leftcontrol")
			case 61: set_name("rightshift")
			case 62: set_name("rightalt")
			case 63: set_name("rightcontrol")
#undef set_name
		}

		if (!strcmp(keymap[code].name, keymap[code].shifted_name))
			strcpy(keymap[code].shifted_name, "");

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
