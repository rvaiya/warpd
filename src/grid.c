/*
 * warpd - A keyboard-driven modal pointer.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include "warpd.h"

static struct grid *g;

/* grid border */
static struct grid *gb;

static void redraw(int gx, int gy, int gw, int gh)
{
	int cx = gx + gw / 2;
	int cy = gy + gh / 2;

	grid_draw(gb, gx, gy, gw, gh);

	gw -= cfg->grid_border_size * 2;
	gh -= cfg->grid_border_size * 2;

	if (gw > 0 && gh > 0) {
		grid_draw(g, gx + cfg->grid_border_size,
			  gy + cfg->grid_border_size, gw, gh);
	} else {
		grid_hide(g);
	}

	cursor_show(cx - cfg->cursor_size / 2, cy - cfg->cursor_size / 2);

	platform_commit();
}

/* returns the terminating input event. */
struct input_event *grid_mode()
{
	struct input_event *ev;
	int		    gx, gy, gw, gh;
	static int	    ogx, ogy, ogw, ogh;

	const int nc = cfg->grid_nc;
	const int nr = cfg->grid_nr;

	/* grid boundaries */

	gx = 0;
	gy = 0;
	screen_get_dimensions(&gw, &gh);

	input_grab_keyboard();
	mouse_hide();
	mouse_reset();
	mouse_move(gw / 2, gh / 2);

	redraw(gx, gy, gw, gh);
	while (1) {
		ev = input_next_event(10);
		int cx, cy;

		size_t i;
		size_t idx = 0;

		mouse_process_key(ev, cfg->grid_up, cfg->grid_down,
				  cfg->grid_left, cfg->grid_right);

		if (!ev || !ev->pressed)
			goto next;

		for (idx = 0; idx < cfg->grid_keys_sz; idx++)
			if (input_event_eq(ev, cfg->grid_keys[idx])) {
				if ((int)idx >= nc * nr)
					continue;

				gy += (gh / nr) * (idx / nc);
				gx += (gw / nc) * (idx % nc);

				gh /= nr;
				gw /= nc;

				gh = gh ? gh : 1;
				gw = gw ? gw : 1;

				mouse_move(gx + (gw / 2), gy + (gh / 2));
			}

		for (i = 0; i < cfg->buttons_sz; i++) {
			if (input_event_eq(ev, cfg->buttons[i]))
				goto exit;
		}

		for (i = 0; i < cfg->oneshot_buttons_sz; i++) {
			if (input_event_eq(ev, cfg->oneshot_buttons[i]))
				goto exit;
		}

		if (input_event_eq(ev, cfg->drag))
			goto exit;

		if (input_event_eq(ev, cfg->exit))
			goto exit;

	next:
		mouse_get_position(&cx, &cy);

		gx = cx - gw / 2;
		gy = cy - gh / 2;

		if (gx != ogx || gy != ogy || gw != ogw || gh != ogh) {
			ogx = gx;
			ogy = gy;
			ogw = gw;
			ogh = gh;

			redraw(gx, gy, gw, gh);
		}
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
