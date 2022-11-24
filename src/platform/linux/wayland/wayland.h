/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */
#ifndef WAYLAND_H
#define WAYLAND_H

#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <strings.h>
#include <cairo/cairo.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "../../../platform.h"
#include "wl/xdg-shell.h"
#include "wl/virtual-pointer.h"
#include "wl/layer-shell.h"
#include "wl/xdg-output.h"


#define MAX_BOXES 64

struct wl {
	struct wl_display *dpy;

	struct wl_shm *shm;
	struct wl_seat *seat;
	struct wl_compositor *compositor;
	struct zwlr_virtual_pointer_v1 *ptr;
	struct zwlr_layer_shell_v1 *layer_shell;
	struct zxdg_output_manager_v1 *xdg_output_manager;
};

struct screen {
	int x;
	int y;
	int w;
	int h;

	int ptrx;
	int ptry;

	int state;

	size_t nr_boxes;
	struct surface *boxes[MAX_BOXES];

	struct surface *overlay;
	struct surface *hints;

	struct wl_output *wl_output;
	struct zxdg_output_v1 *xdg_output;

	struct wl_shm_pool *wl_pool;
	size_t stride;
	cairo_t *cr;
};

struct surface;

struct keymap_entry {
	char name[32];
	char shifted_name[32];
};

extern struct screen screens[MAX_SCREENS];
extern size_t nr_screens;

void add_screen(struct wl_output *output);
int way_hex_to_rgba(const char *str, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);

void init_screen();

struct ptr {
	int x;
	int y;
	struct screen *scr;
};

/* Globals */
extern struct keymap_entry keymap[256];
extern char keynames[256][32];
extern struct ptr ptr;
extern struct wl wl;


/* Surface manipulation */
struct surface *create_surface(struct screen *scr, int x, int y, int w, int h, int capture_input);
void destroy_surface(struct surface *sfc);
struct wl_surface *surface_get_wl_surface(struct surface *sfc);
void surface_show(struct surface *sfc);

/* Exported platform functions. */
void way_run(void (*init)(void));
void way_input_grab_keyboard();
void way_input_ungrab_keyboard();
struct input_event *way_input_next_event(int timeout);
uint8_t way_input_lookup_code(const char *name, int *shifted);
const char *way_input_lookup_name(uint8_t code, int shifted);
struct input_event *way_input_wait(struct input_event *events, size_t sz);
void way_mouse_move(screen_t scr, int x, int y);
void way_mouse_down(int btn);
void way_mouse_up(int btn);
void way_mouse_click(int btn);
void way_mouse_get_position(screen_t *scr, int *x, int *y);
void way_mouse_show();
void way_mouse_hide();
void way_screen_get_dimensions(screen_t scr, int *w, int *h);
void way_screen_draw_box(screen_t scr, int x, int y, int w, int h, const char *color);
void way_screen_clear(screen_t scr);
void way_screen_list(screen_t scr[MAX_SCREENS], size_t *n);
void way_init_hint(const char *bg, const char *fg, int border_radius, const char *font_family);
void way_hint_draw(struct screen *scr, struct hint *hints, size_t n);
void way_scroll(int direction);
void way_copy_selection();
void way_commit();
void way_init();
void init_input();

#endif
