/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */
#include "wayland.h"

static void noop() {}

/* TODO: add support for fractional scaling (requires xdg-output?). */

void layer_surface_handle_configure(void *data,
				    struct zwlr_layer_surface_v1
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

static void surface_handle_enter(void *data,
				 struct wl_surface *wl_surface,
				 struct wl_output *output)
{
	struct surface *sfc = data;
	size_t i;

	for (i = 0; i < nr_screens; i++)
		if (screens[i].wl_output == output)
			sfc->screen = &screens[i];
}

static struct wl_surface_listener wl_surface_listener = {
	.enter = surface_handle_enter,
	.leave = noop,
};

/* TODO: proper cleanup */
void init_surface(struct surface *sfc, int x, int y, int w, int h, int input_focus)
{
	int fd;
	struct wl_shm_pool *pool;
	cairo_surface_t *cairo_surface;
	char shm_path[64];
	static int shm_num = 0;

	sfc->x = x;
	sfc->y = y;
	sfc->w = w;
	sfc->h = h;

	sfc->input_focus = input_focus;

	sfc->stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, w);
	sfc->bufsz = h*sfc->stride;

	sprintf(shm_path, "/warpd_%d", shm_num++);

	fd = shm_open(shm_path, O_CREAT|O_TRUNC|O_RDWR, 0600); //TODO: fixme
	if (fd < 0) {
		perror("shm_open");
		exit(-1);
	}

	ftruncate(fd, sfc->bufsz);

	pool = wl_shm_create_pool(wl.shm, fd, sfc->bufsz);

	sfc->buf = mmap(NULL, sfc->bufsz, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0); //OPT
	sfc->wl_buffer = wl_shm_pool_create_buffer(pool, 0, sfc->w, sfc->h, sfc->stride, WL_SHM_FORMAT_ARGB8888);

	wl_shm_pool_destroy(pool);

	close(fd);

	sfc->wl_surface = NULL;
	sfc->wl_layer_surface = NULL;

	cairo_surface = cairo_image_surface_create_for_data(sfc->buf,
							    CAIRO_FORMAT_ARGB32, sfc->w,
							    sfc->h, sfc->stride);
	sfc->cr = cairo_create(cairo_surface);
}


void surface_destroy(struct surface *sfc)
{
	munmap(sfc->buf, sfc->bufsz);
	wl_surface_destroy(sfc->wl_surface);
	zwlr_layer_surface_v1_destroy(sfc->wl_layer_surface);

	wl_buffer_destroy(sfc->wl_buffer);
}

void surface_hide(struct surface *sfc)
{
	if (sfc->wl_layer_surface) {
		zwlr_layer_surface_v1_destroy(sfc->wl_layer_surface);
		wl_surface_destroy(sfc->wl_surface);

		sfc->wl_surface = NULL;
		sfc->wl_layer_surface = NULL;
	}
}

void surface_show(struct surface *sfc, struct wl_output *output)
{
	if (sfc->wl_surface)
		return;

	sfc->wl_surface = wl_compositor_create_surface(wl.compositor);
	sfc->wl_layer_surface =
		zwlr_layer_shell_v1_get_layer_surface(wl.layer_shell, sfc->wl_surface,
						      output,
						      ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
						      "warpd");

	wl_surface_add_listener(sfc->wl_surface, &wl_surface_listener, sfc);

	assert(sfc->wl_layer_surface);
	zwlr_layer_surface_v1_add_listener(sfc->wl_layer_surface,
					   &layer_surface_listener,
					   sfc);

	zwlr_layer_surface_v1_set_size(sfc->wl_layer_surface, 10, 10);
	zwlr_layer_surface_v1_set_anchor(sfc->wl_layer_surface, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP|ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);
	zwlr_layer_surface_v1_set_margin(sfc->wl_layer_surface, sfc->y, 0, 0, sfc->x);
	zwlr_layer_surface_v1_set_exclusive_zone(sfc->wl_layer_surface, -1);

	if (sfc->input_focus)
		zwlr_layer_surface_v1_set_keyboard_interactivity(sfc->wl_layer_surface,
								 ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE);

	wl_surface_commit(sfc->wl_surface);

	sfc->configured = 0;
	while (!sfc->configured)
		wl_display_dispatch(wl.dpy);

	wl_display_flush(wl.dpy);
}
