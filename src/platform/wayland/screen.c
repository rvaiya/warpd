/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */
#include "wayland.h"

static void noop() {}

static void output_handle_geometry(void *data, struct wl_output *wl_output,
				   int32_t x, int32_t y,
				   int32_t physical_width,
				   int32_t physical_height,
				   int32_t subpixel, const char *make,
				   const char *model, int32_t transform)
{
	struct screen *s = data;
	s->x = x;
	s->y = y;
	s->state++;
}

static void output_handle_mode(void *data,
			       struct wl_output *wl_output,
			       uint32_t flags,
			       int32_t width, int32_t height,
			       int32_t refresh)
{

	struct screen *s = data;
	s->w = width;
	s->h = height;
	s->state++;
}

static struct wl_output_listener output_listener = {
	.geometry = output_handle_geometry,
	.scale = noop,
	.mode = output_handle_mode,
	.done = noop,
};



void init_screens()
{
	int i;
	for (i = 0; i < nr_screens; i++) {
		struct screen *scr = &screens[i];

		wl_output_add_listener(scr->wl_output,
				       &output_listener, scr);

		while (scr->state != 2)
			wl_display_dispatch(wl.dpy);

		wl_output_destroy(scr->wl_output);

		scr->overlay = calloc(1, sizeof(struct surface));
		init_surface(scr->overlay, 0, 0, scr->w, scr->h, 0);
	}
}

void screen_draw_box(struct screen *scr, int x, int y, int w, int h, const char *color)
{
	uint8_t r, g, b, a;

	assert(scr->nr_surfaces < MAX_SURFACES);
	struct surface *sfc = &scr->surfaces[scr->nr_surfaces++];

	init_surface(sfc, x, y, w, h, 0);

	hex_to_rgba(color, &r, &g, &b, &a);
	cairo_set_source_rgba(sfc->cr, r / 255.0, g / 255.0, b / 255.0, a / 255.0);
	cairo_paint(sfc->cr);

	surface_show(sfc);
}


void screen_get_dimensions(struct screen *scr, int *w, int *h)
{
	*w = scr->w;
	*h = scr->h;
}

void screen_clear(struct screen *scr)
{
	int i;
	for (i = 0; i < scr->nr_surfaces; i++)
		surface_destroy(&scr->surfaces[i]);

	scr->nr_surfaces = 0;
	bzero(scr->overlay->buf, scr->overlay->bufsz);
	surface_hide(scr->overlay);
}


