/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */
#include "wayland.h"

static void noop() {}

static void xdg_output_handle_logical_position(void *data,
					       struct zxdg_output_v1
					       *zxdg_output_v1, int32_t x,
					       int32_t y)
{
	struct screen *scr = data;

	scr->x = x;
	scr->y = y;
	scr->state++;
}

static void xdg_output_handle_logical_size(void *data,
					   struct zxdg_output_v1
					   *zxdg_output_v1, int32_t w,
					   int32_t h)
{
	struct screen *scr = data;

	scr->w = w;
	scr->h = h;
	scr->state++;
}

static struct zxdg_output_v1_listener zxdg_output_v1_listener = {
	.logical_position = xdg_output_handle_logical_position,
	.logical_size = xdg_output_handle_logical_size,
	.done = noop,
	.name = noop,
	.description = noop,
};

static void handle_pointer_enter(void *data,
				 struct wl_pointer *wl_pointer,
				 uint32_t serial,
				 struct wl_surface *surface,
				 wl_fixed_t wlx, wl_fixed_t wly)
{
	int i;

	if (!ptr.scr) {
		ptr.x = wl_fixed_to_int(wlx);
		ptr.y = wl_fixed_to_int(wly);

		for (i = 0; i < nr_screens; i++) {
			struct screen *scr = &screens[i];
			if (scr->overlay && surface == surface_get_wl_surface(scr->overlay))
				ptr.scr = scr;
		}
	}
}

static struct wl_pointer_listener wl_pointer_listener = {
	.enter = handle_pointer_enter,
	.leave = noop,
	.motion = noop,
	.button = noop,
	.axis = noop,
	.frame = noop,
	.axis_source = noop,
	.axis_stop = noop,
	.axis_discrete = noop,
};

/* 
 * Register a pointer_listener and listen for enter events after
 * creating a full screen surface for each screen in order to capture the initial
 * cursor position. I couldn't find a better way to achieve this :/.
 */
static void discover_pointer_location()
{
	size_t i;

	wl_pointer_add_listener(wl_seat_get_pointer(wl.seat), &wl_pointer_listener, NULL);

	for (i = 0; i < nr_screens; i++) {
		struct screen *scr = &screens[i];
		scr->overlay = create_surface(scr, 0, 0, scr->w, scr->h, 0);
	}

	wl_display_flush(wl.dpy);
	while (!ptr.scr) {
		/*
		 * Agitate the pointer to precipitate an entry
		 * event. Hyprland appears to require this for
		 * some reason.
		 */
		zwlr_virtual_pointer_v1_motion(wl.ptr, 0,
					       wl_fixed_from_int(1),
					       wl_fixed_from_int(1));

		wl_display_dispatch(wl.dpy);
	}

	for (i = 0; i < nr_screens; i++) {
		struct screen *scr = &screens[i];
		destroy_surface(scr->overlay);
		scr->overlay = NULL;
	}
}

void add_screen(struct wl_output *output)
{
	struct screen *scr = &screens[nr_screens++];
	scr->overlay = NULL;
	scr->wl_output = output;
}

void way_screen_draw_box(struct screen *scr, int x, int y, int w, int h, const char *color)
{
	uint8_t r, g, b, a;

	assert(scr->nr_boxes < MAX_BOXES);

	way_hex_to_rgba(color, &r, &g, &b, &a);
	cairo_set_source_rgba(scr->cr, r / 255.0, g / 255.0, b / 255.0, a / 255.0);
	cairo_rectangle(scr->cr, x, y, w, h);
	cairo_fill(scr->cr);

	scr->boxes[scr->nr_boxes++] = create_surface(scr, x, y, w, h, 0);
}


void way_screen_get_dimensions(struct screen *scr, int *w, int *h)
{
	*w = scr->w;
	*h = scr->h;
}

void way_screen_clear(struct screen *scr)
{
	size_t i;
	for (i = 0; i < scr->nr_boxes; i++)
		destroy_surface(scr->boxes[i]);

	destroy_surface(scr->hints);

	scr->nr_boxes = 0;
	scr->hints = NULL;
}

static void init_screen_pool(struct screen *scr)
{
	int fd;
	static int shm_num = 0;
	char shm_path[64];
	size_t bufsz;
	char *buf;
	cairo_surface_t *cairo_surface;

	scr->stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, scr->w);

	bufsz = scr->stride * scr->h + scr->w * 4;
	sprintf(shm_path, "/warpd_%d", shm_num++);

	fd = shm_open(shm_path, O_CREAT|O_TRUNC|O_RDWR, 0600);
	if (fd < 0) {
		perror("shm_open");
		exit(-1);
	}

	ftruncate(fd, bufsz);

	scr->wl_pool = wl_shm_create_pool(wl.shm, fd, bufsz);
	buf = mmap(NULL, bufsz, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);

	cairo_surface = cairo_image_surface_create_for_data(buf,
							    CAIRO_FORMAT_ARGB32, scr->w,
							    scr->h, scr->stride);
	scr->cr = cairo_create(cairo_surface);
}

void init_screen()
{
	size_t i;

	for (i = 0; i < nr_screens; i++) {
		struct surface *sfc;
		struct screen *scr = &screens[i];

		scr->xdg_output =
		    zxdg_output_manager_v1_get_xdg_output(wl.xdg_output_manager,
							  scr->wl_output);

		zxdg_output_v1_add_listener(scr->xdg_output,
					    &zxdg_output_v1_listener, scr);

		scr->state = 0;
		do {
			wl_display_dispatch(wl.dpy);
		} while (scr->state != 2);

		scr->ptrx = -1;
		scr->ptry = -1;

		init_screen_pool(scr);
	}

	discover_pointer_location();
}
