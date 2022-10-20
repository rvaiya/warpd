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


#define MAX_SURFACES 2048

struct wl {
	struct wl_display *dpy;

	struct wl_shm *shm;
	struct wl_compositor *compositor;
	struct zwlr_virtual_pointer_v1 *ptr;
	struct zwlr_layer_shell_v1 *layer_shell;
	struct zxdg_output_manager_v1 *xdg_output_manager;
};

struct surface {
	int x;
	int y;
	int w;
	int h;

	unsigned char *buf;
	size_t bufsz;
	int stride;
	int dirty;

	int input_focus;
	int configured;

	struct screen *screen;
	struct wl_surface *wl_surface;
	struct wl_buffer *wl_buffer;
	struct zwlr_layer_surface_v1 *wl_layer_surface;

	cairo_t *cr;
};


struct screen {
	int x;
	int y;
	int w;
	int h;

	int ptrx;
	int ptry;

	int state;

	size_t nr_surfaces;
	struct surface surfaces[MAX_SURFACES];

	struct wl_output *wl_output;
	struct zxdg_output_v1 *xdg_output;
	struct surface *overlay;
};

struct keymap_entry {
	char name[32];
	char shifted_name[32];
};

extern struct screen screens[MAX_SCREENS];
extern size_t nr_screens;

extern char keynames[256][32];
extern struct wl wl;

void draw_box(struct screen *scr, int x, int y, int w, int h, const char *color);

void surface_destroy(struct surface *sfc);
void surface_show(struct surface *sfc, struct wl_output *output);
void surface_hide(struct surface *sfc);

void init_surface(struct surface *sfc, int x, int y, int w, int h, int input_focus);

void add_seat(struct wl_seat *seat);
void add_screen(struct wl_output *output);

int wl_hex_to_rgba(const char *str, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);

void init_screen();

extern struct screen *active_screen;
extern struct keymap_entry keymap[256];


void wl_run(void (*init)(void));
void wl_input_grab_keyboard();
void wl_input_ungrab_keyboard();
struct input_event *wl_input_next_event(int timeout);
uint8_t wl_input_lookup_code(const char *name, int *shifted);
const char *wl_input_lookup_name(uint8_t code, int shifted);
struct input_event *wl_input_wait(struct input_event *events, size_t sz);
void wl_mouse_move(screen_t scr, int x, int y);
void wl_mouse_down(int btn);
void wl_mouse_up(int btn);
void wl_mouse_click(int btn);
void wl_mouse_get_position(screen_t *scr, int *x, int *y);
void wl_mouse_show();
void wl_mouse_hide();
void wl_screen_get_dimensions(screen_t scr, int *w, int *h);
void wl_screen_draw_box(screen_t scr, int x, int y, int w, int h, const char *color);
void wl_screen_clear(screen_t scr);
void wl_screen_list(screen_t scr[MAX_SCREENS], size_t *n);
void wl_init_hint(const char *bg, const char *fg, int border_radius, const char *font_family);
void wl_hint_draw(struct screen *scr, struct hint *hints, size_t n);
void wl_scroll(int direction);
void wl_copy_selection();
void wl_commit();


#endif
