/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */
#include <limits.h>
#include "wayland.h"

#define UNIMPLEMENTED { \
	fprintf(stderr, "FATAL: wayland: %s unimplemented\n", __func__); \
	exit(-1);							 \
}

static struct {
	const char *name;
	const char *xname;
} normalization_map[] = {
	{"esc", "Escape"},
	{",", "comma"},
	{".", "period"},
	{"-", "minus"},
	{"/", "slash"},
	{";", "semicolon"},
	{"$", "dollar"},
	{"backspace", "BackSpace"},
};

void init_wl();

void wl_run(void (*loop)(void))
{
	init_wl();

	loop();
}

/* Input */

uint8_t wl_input_lookup_code(const char *name, int *shifted)
{
	size_t i;

	for (i = 0; i < sizeof normalization_map / sizeof normalization_map[0]; i++)
		if (!strcmp(normalization_map[i].name, name))
			name = normalization_map[i].xname;

	for (i = 0; i < 256; i++)
		if (!strcmp(keymap[i].name, name)) {
			*shifted = 0;
			return i;
		} else if (!strcmp(keymap[i].shifted_name, name)) {
			*shifted = 1;
			return i;
		}

	return 0;
}

const char *wl_input_lookup_name(uint8_t code, int shifted)
{
	size_t i;
	const char *name = NULL;

	if (shifted && keymap[code].shifted_name[0])
		name = keymap[code].shifted_name;
	else if (!shifted && keymap[code].name[0])
		name = keymap[code].name;
	
	for (i = 0; i < sizeof normalization_map / sizeof normalization_map[0]; i++)
		if (name && !strcmp(normalization_map[i].xname, name))
			name = normalization_map[i].name;

	return name;
}

void wl_mouse_move(struct screen *scr, int x, int y)
{
	size_t i;
	int maxx = INT_MIN;
	int maxy = INT_MIN;
	int minx = INT_MAX;
	int miny = INT_MAX;

	active_screen->ptrx = x;
	active_screen->ptry = y;

	for (i = 0; i < nr_screens; i++) {
		int x = screens[i].x + screens[i].w;
		int y = screens[i].y + screens[i].h;

		if (screens[i].y < miny)
			miny = screens[i].y;
		if (screens[i].x < minx)
			minx = screens[i].x;

		if (y > maxy)
			maxy = y;
		if (x > maxx)
			maxx = x;
	}

	/*
	 * Virtual pointer space always beings at 0,0, while global compositor
	 * space may have a negative real origin :/.
	 */
	zwlr_virtual_pointer_v1_motion_absolute(wl.ptr, 0,
						wl_fixed_from_int(x+scr->x-minx),
						wl_fixed_from_int(y+scr->y-miny),
						wl_fixed_from_int(maxx-minx),
						wl_fixed_from_int(maxy-miny));
	zwlr_virtual_pointer_v1_frame(wl.ptr);

	active_screen = scr;
	wl_display_flush(wl.dpy);
}

#define normalize_btn(btn) \
	switch (btn) { \
		case 1: btn = 272;break; \
		case 2: btn = 274;break; \
		case 3: btn = 273;break; \
	}

void wl_mouse_down(int btn)
{
	normalize_btn(btn);
	zwlr_virtual_pointer_v1_button(wl.ptr, 0, btn, 1);
}

void wl_mouse_up(int btn)
{
	normalize_btn(btn);
	zwlr_virtual_pointer_v1_button(wl.ptr, 0, btn, 0);
}

void wl_mouse_click(int btn)
{
	normalize_btn(btn);

	zwlr_virtual_pointer_v1_button(wl.ptr, 0, btn, 1);
	zwlr_virtual_pointer_v1_button(wl.ptr, 0, btn, 0);
	zwlr_virtual_pointer_v1_frame(wl.ptr);

	wl_display_flush(wl.dpy);
}

void wl_mouse_get_position(struct screen **scr, int *x, int *y)
{
	if (scr)
		*scr = active_screen;
	if (x)
		*x = active_screen->ptrx;
	if (y)
		*y = active_screen->ptry;
}

void wl_mouse_show()
{
}

void wl_mouse_hide()
{
	fprintf(stderr, "wayland: mouse hiding not implemented\n");
}

void wl_scroll(int direction)
{
	//TODO: add horizontal scroll
	direction = direction == SCROLL_DOWN ? 1 : -1;

	zwlr_virtual_pointer_v1_axis_discrete(wl.ptr, 0, 0,
					      wl_fixed_from_int(15*direction),
					      direction);

	zwlr_virtual_pointer_v1_frame(wl.ptr);

	wl_display_flush(wl.dpy);
}

void wl_copy_selection() { UNIMPLEMENTED }
struct input_event *wl_input_wait(struct input_event *events, size_t sz) { UNIMPLEMENTED }

void wl_screen_list(struct screen *scr[MAX_SCREENS], size_t *n) 
{
	size_t i;
	for (i = 0; i < nr_screens; i++)
		scr[i] = &screens[i];

	*n = nr_screens;
}

void wl_commit()
{
}

void wl_platform_init()
{
	platform.commit = wl_commit;
	platform.copy_selection = wl_copy_selection;
	platform.hint_draw = wl_hint_draw;
	platform.init_hint = wl_init_hint;
	platform.input_grab_keyboard = wl_input_grab_keyboard;
	platform.input_lookup_code = wl_input_lookup_code;
	platform.input_lookup_name = wl_input_lookup_name;
	platform.input_next_event = wl_input_next_event;
	platform.input_ungrab_keyboard = wl_input_ungrab_keyboard;
	platform.input_wait = wl_input_wait;
	platform.mouse_click = wl_mouse_click;
	platform.mouse_down = wl_mouse_down;
	platform.mouse_get_position = wl_mouse_get_position;
	platform.mouse_hide = wl_mouse_hide;
	platform.mouse_move = wl_mouse_move;
	platform.mouse_show = wl_mouse_show;
	platform.mouse_up = wl_mouse_up;
	platform.run = wl_run;
	platform.screen_clear = wl_screen_clear;
	platform.screen_draw_box = wl_screen_draw_box;
	platform.screen_get_dimensions = wl_screen_get_dimensions;
	platform.screen_list = wl_screen_list;
	platform.scroll = wl_scroll;
}
