/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include "warpd.h"

struct hint hints[MAX_HINTS];
struct hint matched[MAX_HINTS];

static size_t nr_hints;
static size_t nr_matched;

static void filter(screen_t scr, const char *s)
{
	size_t i;

	nr_matched = 0;
	for (i = 0; i < nr_hints; i++) {
		if (strstr(hints[i].label, s) == hints[i].label)
			matched[nr_matched++] = hints[i];
	}

	screen_clear(scr);
	hint_draw(scr, matched, nr_matched);
	platform_commit();
}

size_t generate_hints(screen_t scr, struct hint *hints)
{
	int sw, sh;
	int i, j;
	size_t n = 0;

	const char *chars = cfg->hint_chars;
	const int nr = strlen(chars);
	const int nc = strlen(chars);

	screen_get_dimensions(scr, &sw, &sh);

        /* hint_size corresponds to the percentage of column/row space
         * taken up by the hints. At the cost of including math.h, one
         * could replace cfg->hint-size / 100 by its square root, so
         * that hint_size corresponds to the approximate percentage of
         * screen area taken up by the hints
         */

        const int w = sw / nc * cfg->hint_size / 100;
        const int h = sh / nr * cfg->hint_size / 100;

        const int colgap = sw / nc - w;
        const int rowgap = sh / nr - h;

        const int x_offset = (sw - nc * w - (nc - 1) * colgap) / 2;
        const int y_offset = (sh - nr * h - (nr - 1) * rowgap) / 2;

        int x = x_offset;
        int y = y_offset;

        for (i = 0; i < nc; i++) {
		for (j = 0; j < nr; j++) {
			struct hint *hint = &hints[n++];

			hint->x = x;
			hint->y = y;

			hint->w = w;
			hint->h = h;

			hint->label[0] = chars[i];
			hint->label[1] = chars[j];
			hint->label[2] = 0;

			y += rowgap + h;
		}

		y = y_offset;
		x += colgap + w;
	}

	return n;
}

void init_hint_mode()
{
	init_hint(cfg->hint_bgcolor, cfg->hint_fgcolor, cfg->hint_border_radius,
		  cfg->hint_font);
}

int hint_mode()
{
	screen_t scr;

	mouse_get_position(&scr, NULL, NULL);
	nr_hints = generate_hints(scr, hints);

	filter(scr, "");

	int rc = 0;
	char buf[32] = {0};
	input_grab_keyboard();

	mouse_hide();

	while (1) {
		struct input_event *ev;
		ssize_t len;

		ev = input_next_event(0);

		if (!ev->pressed)
			continue;

		len = strlen(buf);

		if (input_event_eq(ev, cfg->hint_exit)) {
			rc = -1;
			break;
		} else if (input_event_eq(ev, "C-u")) {
			buf[0] = 0;
		} else if (input_event_eq(ev, "backspace")) {
			if (len)
				buf[len - 1] = 0;
		} else {
			const char *name = input_event_tostr(ev);

			if (!name || name[1])
				continue;

			buf[len++] = name[0];
		}

		filter(scr, buf);

		if (nr_matched == 1) {
			int nx, ny;
			struct hint *h = &matched[0];

			screen_clear(scr);

			nx = h->x + h->w / 2;
			ny = h->y + h->h / 2;

			/* 
			 * Wiggle the cursor a single pixel to accommodate 
			 * text selection widgets which don't like spontaneous
			 * cursor warping.
			 */
			mouse_move(scr, nx+1, ny+1);

			mouse_move(scr, nx, ny);
			break;
		} else if (nr_matched == 0) {
			break;
		}
	}

	input_ungrab_keyboard();
	screen_clear(scr);
	mouse_show();

	platform_commit();
	return rc;
}
