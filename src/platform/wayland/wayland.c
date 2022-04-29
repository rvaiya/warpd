/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */
#include "wayland.h"

static int ptrx = -1;
static int ptry = -1;

#define UNIMPLEMENTED { \
	fprintf(stderr, "FATAL: wayland: %s unimplemented\n", __func__); \
	exit(-1);							 \
}

void init_wl();

void start_main_loop(void (*loop)(void))
{
	init_wl();

	loop();
}

/* Input */

uint8_t input_lookup_code(const char *name)
{
	size_t i;
	for (i = 0; i < 256; i++)
		if (!strcmp(keynames[i], name))
			return i;

	return 0;
}

const char *input_lookup_name(uint8_t code)
{
	return keynames[code] ? keynames[code] : "UNDEFINED";
}

void mouse_move(struct screen *scr, int x, int y)
{
	int i;
	int maxx = 0;
	int maxy = 0;

	ptrx = x;
	ptry = y;

	for (i = 0; i < nr_screens; i++) {
		int x = screens[i].x + screens[i].w;
		int y = screens[i].y + screens[i].h;

		if (y > maxy)
			maxy = y;
		if (x > maxx)
			maxx = x;
	}

	zwlr_virtual_pointer_v1_motion_absolute(wl.ptr, 0,
						wl_fixed_from_int(x+scr->x),
						wl_fixed_from_int(y+scr->y),
						wl_fixed_from_int(maxx),
						wl_fixed_from_int(maxy));
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

void mouse_down(int btn)
{
	normalize_btn(btn);
	zwlr_virtual_pointer_v1_button(wl.ptr, 0, btn, 1);
}

void mouse_up(int btn)
{
	normalize_btn(btn);
	zwlr_virtual_pointer_v1_button(wl.ptr, 0, btn, 0);
}

void mouse_click(int btn)
{
	normalize_btn(btn);

	zwlr_virtual_pointer_v1_button(wl.ptr, 0, btn, 1);
	zwlr_virtual_pointer_v1_button(wl.ptr, 0, btn, 0);
	zwlr_virtual_pointer_v1_frame(wl.ptr);

	wl_display_flush(wl.dpy);
}

void mouse_get_position(struct screen **scr, int *x, int *y)
{
	//TODO: figure out how to retrieve actual coordinates (if possible)

	if (ptrx == -1) {
		ptrx = active_screen->w/2;
		ptry = active_screen->h/2;
	}

	if (scr)
		*scr = active_screen;
	if (x)
		*x = ptrx;
	if (y)
		*y = ptry;
}

void mouse_show()
{
}

void mouse_hide()
{
	fprintf(stderr, "wayland: mouse hiding not implemented\n");
}

void scroll(int direction)
{
	//TODO: add horizontal scroll
	direction = direction == SCROLL_DOWN ? 1 : -1;

	zwlr_virtual_pointer_v1_axis_discrete(wl.ptr, 0, 0,
					      wl_fixed_from_int(15*direction),
					      direction);

	zwlr_virtual_pointer_v1_frame(wl.ptr);

	wl_display_flush(wl.dpy);
}

void copy_selection() { UNIMPLEMENTED }
struct input_event *input_wait(struct input_event *events, size_t sz) { UNIMPLEMENTED }

void screen_list(struct screen *scr[MAX_SCREENS], size_t *n) 
{
	int i;
	for (i = 0; i < nr_screens; i++)
		scr[i] = &screens[i];

	*n = nr_screens;
}

void platform_commit()
{
}
