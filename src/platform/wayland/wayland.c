/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */
#include "wayland.h"

#define UNIMPLEMENTED { \
	fprintf(stderr, "FATAL: wayland: %s unimplemented\n", __func__); \
	exit(-1);							 \
}

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
	{"$", "dollar"},
	{"backspace", "BackSpace"},
};

void init_wl();

void platform_run(void (*loop)(void))
{
	init_wl();

	loop();
}

/* Input */

uint8_t platform_input_lookup_code(const char *name, int *shifted)
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

const char *platform_input_lookup_name(uint8_t code, int shifted)
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

void platform_mouse_move(struct screen *scr, int x, int y)
{
	int i;
	int maxx = 0;
	int maxy = 0;
	int minx = 0;
	int miny = 0;

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

void platform_mouse_down(int btn)
{
	normalize_btn(btn);
	zwlr_virtual_pointer_v1_button(wl.ptr, 0, btn, 1);
}

void platform_mouse_up(int btn)
{
	normalize_btn(btn);
	zwlr_virtual_pointer_v1_button(wl.ptr, 0, btn, 0);
}

void platform_mouse_click(int btn)
{
	normalize_btn(btn);

	zwlr_virtual_pointer_v1_button(wl.ptr, 0, btn, 1);
	zwlr_virtual_pointer_v1_button(wl.ptr, 0, btn, 0);
	zwlr_virtual_pointer_v1_frame(wl.ptr);

	wl_display_flush(wl.dpy);
}

void platform_mouse_get_position(struct screen **scr, int *x, int *y)
{
	if (scr)
		*scr = active_screen;
	if (x)
		*x = active_screen->ptrx;
	if (y)
		*y = active_screen->ptry;
}

void platform_mouse_show()
{
}

void platform_mouse_hide()
{
	fprintf(stderr, "wayland: mouse hiding not implemented\n");
}

void platform_scroll(int direction)
{
	//TODO: add horizontal scroll
	direction = direction == SCROLL_DOWN ? 1 : -1;

	zwlr_virtual_pointer_v1_axis_discrete(wl.ptr, 0, 0,
					      wl_fixed_from_int(15*direction),
					      direction);

	zwlr_virtual_pointer_v1_frame(wl.ptr);

	wl_display_flush(wl.dpy);
}

void platform_copy_selection() { UNIMPLEMENTED }
struct input_event *platform_input_wait(struct input_event *events, size_t sz) { UNIMPLEMENTED }

void platform_screen_list(struct screen *scr[MAX_SCREENS], size_t *n) 
{
	int i;
	for (i = 0; i < nr_screens; i++)
		scr[i] = &screens[i];

	*n = nr_screens;
}

void platform_commit()
{
}
