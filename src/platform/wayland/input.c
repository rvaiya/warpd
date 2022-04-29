/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */
#include "wayland.h"

static struct input_event input_queue[32];
static size_t input_queue_sz;
char keynames[256][32];

static struct surface input_pixel;
static uint8_t active_mods = 0;

static void noop() {}

void update_mods(uint8_t code, uint8_t pressed)
{
	const char *name = input_lookup_name(code);

	if (strstr(name, "Control") == name) {
		if (pressed)
			active_mods |= MOD_CONTROL;
		else
			active_mods &= ~MOD_CONTROL;
	}

	if (strstr(name, "Shift") == name) {
		if (pressed)
			active_mods |= MOD_SHIFT;
		else
			active_mods &= ~MOD_SHIFT;
	}

	if (strstr(name, "Super") == name) {
		if (pressed)
			active_mods |= MOD_META;
		else
			active_mods &= ~MOD_META;
	}

	if (strstr(name, "Alt") == name) {
		if (pressed)
			active_mods |= MOD_ALT;
		else
			active_mods &= ~MOD_ALT;
	}
}


static void handle_key(void *data,
		       struct wl_keyboard *wl_keyboard,
		       uint32_t serial,
		       uint32_t time, uint32_t code, uint32_t state)
{
	struct input_event *ev = &input_queue[input_queue_sz++];

	update_mods(code, state);

	ev->code = code;
	ev->pressed = state;
	ev->mods = active_mods;
}

static void handle_keymap(void *data,
			  struct wl_keyboard *wl_keyboard,
			  uint32_t format, int32_t fd, uint32_t size)
{
	size_t i;
	char *buf;
	struct xkb_context *ctx;
	struct xkb_keymap *xkbmap;
	struct xkb_state *xkbstate;

	struct {
		const char *name;
		const char *alias;
	} aliases[] = {
		{"Escape", "esc"},
		{"minus", "-"},
		{"comma", ","},
		{"slash", "/"},
		{"period", "."},
		{"BackSpace", "backspace"},
		{"bracketleft", "["},
		{"bracketright", "]"},
		{"semicolon", ";"},
		{"quote", "'"},
		{"apostrophe", "'"},
	};

	assert(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1);

	buf = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	assert(buf);

	ctx = xkb_context_new(0);
	assert(ctx);

	xkbmap =
	    xkb_keymap_new_from_string(ctx, buf,
				       XKB_KEYMAP_FORMAT_TEXT_V1, 0);

	assert(xkbmap);

	xkbstate = xkb_state_new(xkbmap);
	assert(xkbstate);

	for (i = 0; i < 248; i++) {
		size_t j;
		keynames[i][0] = 0;
		xkb_keysym_t sym = xkb_state_key_get_one_sym(xkbstate, i+8);
		xkb_keysym_get_name(sym, keynames[i], sizeof keynames[i]);

		for (j = 0; j < sizeof(aliases)/sizeof(aliases[0]); j++) {
			if (!strcmp(aliases[j].name, keynames[i]))
				strcpy(keynames[i], aliases[j].alias);
		}
	}

	xkb_state_unref(xkbstate);
	xkb_keymap_unref(xkbmap);
}

static struct wl_keyboard_listener wl_keyboard_listener = {
	.key = handle_key,
	.keymap = handle_keymap,
	.enter = noop,
	.leave = noop,
	.modifiers = noop,
	.repeat_info = noop,
};

void input_ungrab_keyboard()
{
	surface_hide(&input_pixel);
}

/*
 * *Note*: Wayland does not allow for global input handlers.
 * Input events can only be captured by a focused window :(.
 */
void input_grab_keyboard()
{
	if (!input_pixel.buf)
		init_surface(&input_pixel, -1, -1, 1, 1, 1);

	surface_show(&input_pixel, NULL);
}

struct input_event *input_next_event(int timeout)
{
	static struct input_event ev;

	struct pollfd pfd = {
		.fd = wl_display_get_fd(wl.dpy),
		.events = POLLIN,
	};

	while (1) {
		wl_display_dispatch_pending(wl.dpy);
		if (input_queue_sz) {
			input_queue_sz--;
			ev = input_queue[0];
			memcpy(input_queue, input_queue+1, sizeof(struct input_event)*input_queue_sz);

			return &ev;
		}

		if (!poll(&pfd, 1, timeout ? timeout : -1))
			return NULL;

		wl_display_dispatch(wl.dpy);
	}
}

void add_seat(struct wl_seat *seat) 
{
	wl_keyboard_add_listener(wl_seat_get_keyboard(seat), &wl_keyboard_listener, NULL);
	wl_seat_destroy(seat);
}
