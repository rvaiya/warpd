/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */
#include "wayland.h"

struct screen *active_screen = NULL;

static void noop() {}

void xdg_output_handle_logical_position(void *data,
					struct zxdg_output_v1 *zxdg_output_v1,
					int32_t x,
					int32_t y)
{
	struct screen *scr = data;

	scr->x = x;
	scr->y = y;
	scr->state++;
}

void xdg_output_handle_logical_size(void *data,
				    struct zxdg_output_v1 *zxdg_output_v1,
				    int32_t w,
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

void add_screen(struct wl_output *output)
{
	struct screen *scr = &screens[nr_screens++];
	scr->wl_output = output;
}

void wl_screen_draw_box(struct screen *scr, int x, int y, int w, int h, const char *color)
{
	uint8_t r, g, b, a;

	assert(scr->nr_surfaces < MAX_SURFACES);
	struct surface *sfc = &scr->surfaces[scr->nr_surfaces++];

	init_surface(sfc, x, y, w, h, 0);

	wl_hex_to_rgba(color, &r, &g, &b, &a);
	cairo_set_source_rgba(sfc->cr, r / 255.0, g / 255.0, b / 255.0, a / 255.0);
	cairo_paint(sfc->cr);

	surface_show(sfc, scr->wl_output);
}


void wl_screen_get_dimensions(struct screen *scr, int *w, int *h)
{
	*w = scr->w;
	*h = scr->h;
}

void wl_screen_clear(struct screen *scr)
{
	size_t i;
	for (i = 0; i < scr->nr_surfaces; i++)
		surface_destroy(&scr->surfaces[i]);

	scr->nr_surfaces = 0;
	bzero(scr->overlay->buf, scr->overlay->bufsz);
	surface_hide(scr->overlay);
}

void init_screen()
{
	size_t i;
	struct surface sfc;

	for (i = 0; i < nr_screens; i++) {
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
		scr->overlay = calloc(1, sizeof(struct surface));
		init_surface(scr->overlay, 0, 0, scr->w, scr->h, 0);
	}

	sfc.screen = 0;
	init_surface(&sfc, 0, 0, 1, 1, 0);
	surface_show(&sfc, NULL);
	while(!sfc.screen)
		wl_display_dispatch(wl.dpy);

	active_screen = sfc.screen;
	surface_destroy(&sfc);

	surface_show(active_screen->overlay, active_screen->wl_output);
	while (active_screen->ptrx == -1)
		wl_display_dispatch(wl.dpy);
	surface_hide(active_screen->overlay);
}
