/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */
#include "wayland.h"

static struct input_event input_queue[32];
static size_t input_queue_sz;

static uint8_t x_active_mods = 0;

static void noop() {}
struct keymap_entry keymap[256] = {0};

static void update_mods(uint8_t code, uint8_t pressed)
{
	const char *name = way_input_lookup_name(code, 0);

	if (!name)
		return;

	if (strstr(name, "Control") == name) {
		if (pressed)
			x_active_mods |= PLATFORM_MOD_CONTROL;
		else
			x_active_mods &= ~PLATFORM_MOD_CONTROL;
	}

	if (strstr(name, "Shift") == name) {
		if (pressed)
			x_active_mods |= PLATFORM_MOD_SHIFT;
		else
			x_active_mods &= ~PLATFORM_MOD_SHIFT;
	}

	if (strstr(name, "Super") == name) {
		if (pressed)
			x_active_mods |= PLATFORM_MOD_META;
		else
			x_active_mods &= ~PLATFORM_MOD_META;
	}

	if (strstr(name, "Alt") == name) {
		if (pressed)
			x_active_mods |= PLATFORM_MOD_ALT;
		else
			x_active_mods &= ~PLATFORM_MOD_ALT;
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
	ev->mods = x_active_mods;
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

	assert(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1);

	buf = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	assert(buf);

	ctx = xkb_context_new(0);
	assert(ctx);

	xkbmap = xkb_keymap_new_from_string(ctx, buf, XKB_KEYMAP_FORMAT_TEXT_V1, 0);

	assert(xkbmap);
	xkbstate = xkb_state_new(xkbmap);
	assert(xkbstate);

	for (i = 0; i < 248; i++) {
		const xkb_keysym_t *syms;
		if (xkb_keymap_key_get_syms_by_level(xkbmap, i+8,
						     xkb_state_key_get_layout(xkbstate, i+8),
						     0, &syms)) {
			xkb_keysym_get_name(syms[0], keymap[i].name, sizeof keymap[i].name);
		}

		if (xkb_keymap_key_get_syms_by_level(xkbmap, i+8,
						     xkb_state_key_get_layout(xkbstate, i+8),
						     1,
						     &syms)) {
			xkb_keysym_get_name(syms[0], keymap[i].shifted_name, sizeof keymap[i].shifted_name);
		}
	}
	xkb_state_unref(xkbstate);
	xkb_keymap_unref(xkbmap);
	xkb_context_unref(ctx);
}

static int input_grabbed = 0;
static void handle_enter(void *data,
			 struct wl_keyboard *wl_keyboard,
			 uint32_t serial,
			 struct wl_surface *surface,
			 struct wl_array *keys)
{
	input_grabbed = 1;
}

static void handle_leave(void *data,
			 struct wl_keyboard *wl_keyboard,
			 uint32_t serial,
			 struct wl_surface *surface)
{
	input_grabbed = 0;
}


static struct wl_keyboard_listener wl_keyboard_listener = {
	.key = handle_key,
	.keymap = handle_keymap,
	.enter = handle_enter,
	.leave = handle_leave,
	.modifiers = noop,
	.repeat_info = noop,
};

static struct surface *input_surface = NULL;
/*
 * *Note*: Wayland does not allow for global input handlers.
 * Input events can only be captured by a focused window :(.
 */
void way_input_grab_keyboard()
{
	input_surface = create_surface(&screens[0], -1, -1, 1, 1, 1);

	wl_display_flush(wl.dpy);
	input_grabbed = 0;
	while (!input_grabbed)
		wl_display_dispatch(wl.dpy);
}

void way_input_ungrab_keyboard()
{
	destroy_surface(input_surface);

	while (!input_grabbed)
		wl_display_dispatch(wl.dpy);

	input_surface = NULL;
}


struct input_event *way_input_next_event(int timeout)
{
	static struct input_event ev;

	struct pollfd pfds[] = {
		{ .fd = wl_display_get_fd(wl.dpy), .events = POLLIN },
	};

	while (1) {
		wl_display_flush(wl.dpy);
		wl_display_dispatch_pending(wl.dpy);
		if (input_queue_sz) {
			input_queue_sz--;
			ev = input_queue[0];
			memcpy(input_queue, input_queue+1, sizeof(struct input_event)*input_queue_sz);

			return &ev;
		}

		if (!poll(pfds, sizeof pfds / sizeof pfds[0], timeout ? timeout : -1))
			return NULL;

		wl_display_dispatch(wl.dpy);
	}
}

void init_input()
{
	wl_keyboard_add_listener(wl_seat_get_keyboard(wl.seat), &wl_keyboard_listener, NULL);
}
