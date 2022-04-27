/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */
#include "wayland.h"

static int ptrx = 0;
static int ptry = 0;

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
	ptrx = x;
	ptry = y;

	zwlr_virtual_pointer_v1_motion_absolute(wl.ptr, 0,
						wl_fixed_from_int(x),
						wl_fixed_from_int(y),
						wl_fixed_from_int(scr->w),
						wl_fixed_from_int(scr->h));
	zwlr_virtual_pointer_v1_frame(wl.ptr);



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

	if (scr)
		*scr = &screens[0];
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
void screen_list(struct screen *scr[MAX_SCREENS], size_t *n) { UNIMPLEMENTED }

void platform_commit()
{
}
