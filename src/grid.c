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
		platform_screen_draw_box(scr, x, y+(ygap+sz)*i, w, sz, color);

	for (i = 0; i < nc+1; i++)
		platform_screen_draw_box(scr, x+(xgap+sz)*i, y, sz, h, color);
}

static void redraw(int mx, int my, int force)
{
	const int x = mx - grid_width/2;
	const int y = my - grid_height/2;

	const int nc = config_get_int("grid_nc");
	const int nr = config_get_int("grid_nr");
	const int cursz = config_get_int("cursor_size");
	const int gsz = config_get_int("grid_size");
	const int gbsz = config_get_int("grid_border_size");
	const char *gbcol = config_get("grid_border_color");
	const char *gcol = config_get("grid_color");

	const int gh = grid_height;
	const int gw = grid_width;

	static int omx, omy;

	/* Avoid unnecessary redraws. */
	if (!force && omx == mx && omy == my)
		return;

	omx = mx;
	omy = my;

	platform_screen_clear(scr);

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

	platform_screen_draw_box(scr,
			x+gw/2-cursz/2, y+gh/2-cursz/2,
			cursz, cursz,
			config_get("cursor_color"));

	platform_commit();
}

/* Returns the terminating input event. */
struct input_event *grid_mode()
{
	int mx, my;
	struct input_event *ev;

	const int nc = config_get_int("grid_nc");
	const int nr = config_get_int("grid_nr");

	platform_input_grab_keyboard();
	platform_mouse_hide();
	mouse_reset();

	platform_mouse_get_position(&scr, NULL, NULL);
	platform_screen_get_dimensions(scr, &grid_width, &grid_height);

	mx = grid_width / 2;
	my = grid_height / 2;
	platform_mouse_move(scr, mx, my);
	redraw(mx, my, 1);

	while (1) {
		int idx;

		ev = platform_input_next_event(10);
		platform_mouse_get_position(NULL, &mx, &my);

		if (mouse_process_key(ev, "grid_up", "grid_down", "grid_left", "grid_right")) {
			redraw(mx, my, 0);
			continue;
		}

		/* Timeout */
		if (!ev || !ev->pressed)
			continue;

		if ((idx = config_input_match(ev, "grid_keys", 1)) && idx <= nc * nr) {
			my = (my - grid_height / 2) + (grid_height / nr) * ((idx-1) / nc);
			mx = (mx - grid_width / 2) + (grid_width / nc) * ((idx-1) % nc);

			grid_height /= nr;
			grid_width /= nc;
			mx += grid_width / 2;
			my += grid_height / 2;

			platform_mouse_move(scr, mx, my);
			redraw(mx, my, 0);
		}

		if (config_input_match(ev, "buttons", 0) ||
			config_input_match(ev, "oneshot_buttons", 0))
			goto exit;

		if (config_input_match(ev, "grid", 1) ||
		    config_input_match(ev, "hint", 1) ||
		    config_input_match(ev, "exit", 1) ||
		    config_input_match(ev, "drag", 1) ||
		    config_input_match(ev, "grid_exit", 1))
			goto exit;

		redraw(mx, my, 0);
	}

exit:
	platform_screen_clear(scr);
	platform_mouse_show();

	platform_input_ungrab_keyboard();

	platform_commit();
	return ev;
}
