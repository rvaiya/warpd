/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include "warpd.h"
#include <time.h>

/* constants */

static double v0, vf, a, a0, a1;
static int inc = 0;
static int sw, sh;

/* state */

static int left = 0;
static int right = 0;
static int up = 0;
static int down = 0;

static int resting = 1;

static screen_t scr = 0;
static double cx = 0;
static double cy = 0;

static double v = 0;
static int opnum = 0;

static long get_time_us()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (ts.tv_nsec / 1E3) + (ts.tv_sec * 1E6);
}

static int tonum(uint8_t code)
{
	const char *name = input_lookup_name(code);

	if (!name)
		return -1;

	if (name[0] > '9' || name[0] < '0')
		return -1;

	return name[0] - '0';
}

static void update_cursor_position()
{
	int ix, iy;

	mouse_get_position(&scr, &ix, &iy);
	screen_get_dimensions(scr, &sw, &sh);

	cx = (double)ix;
	cy = (double)iy;
}

static void tick()
{
	static long last_update = 0;

	const long t = get_time_us();
	const double elapsed = (double)(t - last_update) / 1E3;
	last_update = t;

	const double dx = right - left;
	const double dy = down - up;

	const int maxx = sw - cfg->cursor_size;
	const int maxy = sh - cfg->cursor_size/2;
	const int miny = cfg->cursor_size/2;
	const int minx = 1;


	if (!dx && !dy) {
		resting = 1;
		return;
	}

	if (resting) {
		update_cursor_position();
		v = v0;
		resting = 0;
	}

	cx += v * elapsed * dx;
	cy += v * elapsed * dy;

	v += elapsed * a;
	if (v > vf)
		v = vf;

	cx = cx < minx ? minx : cx;
	cy = cy < miny ? miny : cy;
	cy = cy > maxy ? maxy : cy;
	cx = cx > maxx ? maxx : cx;

	mouse_move(scr, cx, cy);
}

/*
 * The function to which continuous cursor movement is delegated for grid and
 * normal mode. Expects to be called every 10ms or so.
 *
 * mouse_reset() should be called at the beginning of the containing event
 * loop.
 *
 * Returns 1 if the cursor position was updated.
 */

int mouse_process_key(struct input_event *ev,
		      const char *up_key,
		      const char *down_key,
		      const char *left_key,
		      const char *right_key)
{
	int ret = 0;
	int n;

	/* timeout */
	if (!ev) {
		tick();
		return left || right || up || down;
	}

	if ((n = tonum(ev->code)) != -1 && ev->mods == 0) {
		if (ev->pressed)
			opnum = opnum * 10 + n;

		/* Allow 0 on its own to propagate as a special case. */
		if (opnum == 0)
			return 0;
		else
			return 1;
	}

	if (input_event_eq(ev, down_key)) {
		down = ev->pressed;
		ret = 1;
	} else if (input_event_eq(ev, left_key)) {
		left = ev->pressed;
		ret = 1;
	} else if (input_event_eq(ev, right_key)) {
		right = ev->pressed;
		ret = 1;
	} else if (input_event_eq(ev, up_key)) {
		up = ev->pressed;
		ret = 1;
	}

	if (opnum && ret) {
		const int x = right - left;
		const int y = down - up;

		update_cursor_position();

		cx += inc * opnum * x;
		cy += inc * opnum * y;

		mouse_move(scr, cx, cy);

		opnum = 0;

		left = 0;
		right = 0;
		up = 0;
		down = 0;

		return 1;
	}

	tick();
	return ret;
}

void mouse_fast()
{
	a = a1;
}

void mouse_slow()
{
	v = v0;
	a = a0;
}

void mouse_reset()
{
	opnum = 0;
	left = 0;
	right = 0;
	up = 0;
	down = 0;
	a = a0;

	update_cursor_position();

	tick();
}

void init_mouse()
{
	inc = 15; // TODO: make this configurable

	/* pixels/ms */

	v0 = (double)cfg->speed / 1000.0;
	vf = (double)cfg->max_speed / 1000.0;
	a0 = (double)cfg->acceleration / 1000000.0;
	a1 = (double)cfg->accelerator_acceleration / 1000000.0;

	a = a0;
}
