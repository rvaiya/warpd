/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */
#ifndef WAYLAND_H
#define WAYLAND_H

#define _XOPEN_SOURCE 500

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

#include "../../platform.h"
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

	int state;

	size_t nr_surfaces;
	struct surface surfaces[MAX_SURFACES];

	struct wl_output *wl_output;
	struct zxdg_output_v1 *xdg_output;
	struct surface *overlay;
};

extern struct screen screens[MAX_SCREENS];
extern int nr_screens;

extern char keynames[256][32];
extern struct wl wl;

void draw_box(struct screen *scr, int x, int y, int w, int h, const char *color);

void surface_destroy(struct surface *sfc);
void surface_show(struct surface *sfc, struct wl_output *output);
void surface_hide(struct surface *sfc);

void init_surface(struct surface *sfc, int x, int y, int w, int h, int input_focus);

void add_seat(struct wl_seat *seat);
void add_screen(struct wl_output *output);

int hex_to_rgba(const char *str, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);

void init_screen();
extern struct screen *active_screen;

#endif
