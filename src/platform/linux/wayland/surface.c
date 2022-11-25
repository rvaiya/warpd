/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */
#include "wayland.h"

static void noop() {}

/* TODO: add support for fractional scaling (requires xdg-output?). */

/* A 'surface' in the local context can be thought of as a viewport into a
 * rectangular region of the screen's backing buffer (the wl_shm_pool).  It is
 * undergirded by a corresponding wayland surface and wayland layer surface with a
 * wayland buffer object created from the relevant part of the screen's memory
 * pool. Surfaces are visible as long as they exist, and hiding them is achieved
 * by destroying them.
 */
struct surface {
	struct zwlr_layer_surface_v1 *wl_layer_surface;
	struct wl_surface *wl_surface;
	struct wl_buffer *wl_buffer;

	int configured;
	int destroyed;
};

static void layer_surface_handle_configure(void *data, struct zwlr_layer_surface_v1
					   *layer_surface, uint32_t serial,
					   uint32_t width, uint32_t height)
{
	struct surface *sfc = data;

	// Notify the server that we are still alive
	// (i.e a heartbeat).
	zwlr_layer_surface_v1_ack_configure(layer_surface, serial);

	// The protocol requires us to wait for the first configure call before
	// attaching the buffer to the underlying wl_surface object for the
	// first time.

	wl_surface_attach(sfc->wl_surface, sfc->wl_buffer, 0, 0);
	wl_surface_commit(sfc->wl_surface);

	sfc->configured = 1;
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
        .configure = layer_surface_handle_configure,
        .closed = noop,
};

void destroy_surface(struct surface *sfc)
{
	if (sfc) {
		zwlr_layer_surface_v1_destroy(sfc->wl_layer_surface);
		wl_surface_destroy(sfc->wl_surface);
		wl_buffer_destroy(sfc->wl_buffer);

		free(sfc);
	}
}

/* All surfaces which exist are visible. Surface creation/destruction is the means
 * of displaying content on the screen (there is no show/hide) and should be considered a cheap operation
 * which operates on a persistent shared buffer (memory pool). */

struct surface *create_surface(struct screen *scr, int x, int y, int w, int h, int capture_input)
{
	struct surface *sfc = calloc(1, sizeof (struct surface));

	if (x < 0) {
		x = 0;
		w += x;
	}
	if (y < 0) {
		y = 0;
		h += y;
	}
	if ((x+w) > scr->w)
		x = scr->w-w;
	if ((y+h) > scr->h)
		y = scr->h-h;

	sfc->wl_buffer = wl_shm_pool_create_buffer(scr->wl_pool, y*scr->stride + x*4, w, h, scr->stride, WL_SHM_FORMAT_ARGB8888);
	assert(sfc->wl_buffer);
	sfc->wl_surface = wl_compositor_create_surface(wl.compositor);

	assert(sfc->wl_surface);
	sfc->wl_layer_surface =
		zwlr_layer_shell_v1_get_layer_surface(wl.layer_shell, sfc->wl_surface,
						      scr->wl_output,
						      ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
						      "warpd");

	assert(sfc->wl_layer_surface);

	zwlr_layer_surface_v1_set_size(sfc->wl_layer_surface, 10, 10);
	zwlr_layer_surface_v1_set_anchor(sfc->wl_layer_surface, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP|ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);
	zwlr_layer_surface_v1_set_margin(sfc->wl_layer_surface, y, 0, 0, x);
	zwlr_layer_surface_v1_set_exclusive_zone(sfc->wl_layer_surface, -1);

	zwlr_layer_surface_v1_add_listener(sfc->wl_layer_surface, &layer_surface_listener, sfc);

	sfc->configured = 0;

	if (capture_input) {
		zwlr_layer_surface_v1_set_keyboard_interactivity(sfc->wl_layer_surface,
								  ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE);
	}

	wl_surface_commit(sfc->wl_surface);

	return sfc;
}

struct wl_surface *surface_get_wl_surface(struct surface *sfc)
{
	return sfc->wl_surface;
}
