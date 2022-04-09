/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include "warpd.h"

static int grid_width;
static int grid_height;
static screen_t scr;

static void draw_grid(screen_t scr,
		      const char *color, int sz,
		      int nc, int nr,
		      int x, int y, int w, int h)
{
	int i;

	const double ygap = (h - ((nr+1)*sz))/(double)nr;
	const double xgap = (w - ((nc+1)*sz))/(double)nc;

	if (xgap < 0 || ygap < 0)
		return;

	for (i = 0; i < nr+1; i++)
		screen_draw_box(scr, x, y+(ygap+sz)*i, w, sz, color);

	for (i = 0; i < nc+1; i++)
		screen_draw_box(scr, x+(xgap+sz)*i, y, sz, h, color);
}

static void redraw(int mx, int my, int force)
{
	const int x = mx - grid_width/2;
	const int y = my - grid_height/2;

	const int nc = cfg->grid_nc;
	const int nr = cfg->grid_nr;
	const int cursz = cfg->cursor_size;
	const int gsz = cfg->grid_size;
	const int gbsz = cfg->grid_border_size;
	const char *gbcol = cfg->grid_border_color;
	const char *gcol = cfg->grid_color;

	const int gh = grid_height;
	const int gw = grid_width;

	static int omx, omy;

	/* Avoid unnecessary redraws. */
	if (!force && omx == mx && omy == my)
		return;

	omx = mx;
	omy = my;

	screen_clear(scr);

	/* Draw the border. */
	draw_grid(scr, gbcol,
		  gsz+gbsz*2,
		  nc, nr,
		  x, y, gw, gh);

	/* Draw the grid. */
	draw_grid(scr, gcol,
		  gsz, nc, nr,
		  x+gbsz, y+gbsz,
		  gw-gbsz*2, gh-gbsz*2);

	screen_draw_box(scr,
			x+gw/2-cursz/2, y+gh/2-cursz/2,
			cursz, cursz,
			cfg->cursor_color);

	platform_commit();
}

/* Returns the terminating input event. */
struct input_event *grid_mode()
{
	int mx, my;
	struct input_event *ev;

	const int nc = cfg->grid_nc;
	const int nr = cfg->grid_nr;

	input_grab_keyboard();
	mouse_hide();
	mouse_reset();

	mouse_get_position(&scr, NULL, NULL);
	screen_get_dimensions(scr, &grid_width, &grid_height);

	mx = grid_width / 2;
	my = grid_height / 2;
	mouse_move(scr, mx, my);
	redraw(mx, my, 1);

	while (1) {
		size_t i;
		size_t idx = 0;

		ev = input_next_event(10);
		mouse_get_position(NULL, &mx, &my);


		if (mouse_process_key(ev, cfg->grid_up, cfg->grid_down,
				      cfg->grid_left, cfg->grid_right)) {
			redraw(mx, my, 0);
			continue;
		}

		/* Timeout */
		if (!ev || !ev->pressed)
			continue;

		for (idx = 0; idx < cfg->grid_keys_sz; idx++)
			if (input_event_eq(ev, cfg->grid_keys[idx])) {
				if ((int)idx >= nc * nr)
					continue;

				my = (my - grid_height/2) + (grid_height / nr) * (idx / nc);
				mx = (mx - grid_width/2) + (grid_width / nc) * (idx % nc);

				grid_height /= nr;
				grid_width /= nc;
				mx += grid_width/2;
				my += grid_height/2;

				mouse_move(scr, mx, my);
				redraw(mx, my, 0);
			}

		for (i = 0; i < cfg->buttons_sz; i++) {
			if (input_event_eq(ev, cfg->buttons[i]))
				goto exit;
		}

		for (i = 0; i < cfg->oneshot_buttons_sz; i++) {
			if (input_event_eq(ev, cfg->oneshot_buttons[i]))
				goto exit;
		}

		if (input_event_eq(ev, cfg->grid) ||
		    input_event_eq(ev, cfg->hint) ||
		    input_event_eq(ev, cfg->exit) ||
		    input_event_eq(ev, cfg->drag) ||
		    input_event_eq(ev, cfg->grid_exit))
			goto exit;

		redraw(mx, my, 0);
	}

exit:
	screen_clear(scr);
	mouse_show();

	input_ungrab_keyboard();

	platform_commit();
	return ev;
}
