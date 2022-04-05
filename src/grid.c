/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include "warpd.h"

static struct grid *g;

/* grid border */
static struct grid *gb;

/* state */
static	int	grid_width;
static	int	grid_height;
static screen_t scr;

static void redraw(int mx, int my, int force)
{
	const int x = mx - grid_width/2;
	const int y = my - grid_height/2;

	static int omx, omy;

	/* Avoid unnecessary redraws. */
	if (!force && omx == mx && omy == my)
		return;

	omx = mx;
	omy = my;

	/* Inner grid dimensions. */
	const int gx = x + cfg->grid_border_size;
	const int gy = y + cfg->grid_border_size;
	const int gw = grid_width - cfg->grid_border_size*2;
	const int gh = grid_height - cfg->grid_border_size*2;

	grid_draw(gb, scr, x, y, grid_width, grid_height);
	grid_draw(g, scr, gx, gy, gw, gh);
	cursor_draw(scr, mx, my);

	platform_commit();
}

/* Returns the terminating input event. */
struct input_event *grid_mode()
{
	int mx, my;
	struct	input_event *ev;

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
	grid_hide(g);
	grid_hide(gb);
	mouse_show();

	input_ungrab_keyboard();

	platform_commit();
	return ev;
}

void init_grid_mode()
{
	g = create_grid(cfg->grid_color, cfg->grid_size, cfg->grid_nc,
			cfg->grid_nr);

	gb = create_grid(cfg->grid_border_color,
			 cfg->grid_size + cfg->grid_border_size * 2,
			 cfg->grid_nc, cfg->grid_nr);
}
