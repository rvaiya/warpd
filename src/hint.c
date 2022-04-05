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

static void filter(const char *s)
{
	size_t i;

	nr_matched = 0;
	for (i = 0; i < nr_hints; i++) {
		if (strstr(hints[i].label, s) == hints[i].label)
			matched[nr_matched++] = hints[i];
	}
}

size_t generate_hints(screen_t scr, struct hint *hints)
{
	int    sw, sh;
	int    i, j;
	size_t n = 0;

	const char *chars = cfg->hint_chars;
	const int   nr = strlen(chars);
	const int   nc = strlen(chars);

	screen_get_dimensions(scr, &sw, &sh);

	const int hint_size = cfg->hint_size * sh / 1080;

	const int colgap = (sw - (hint_size * 2)) / (nc - 1);
	const int rowgap = (sh - (hint_size * 2)) / (nr - 1);

	const int w = hint_size * 1.6;
	const int h = hint_size;

	for (i = 0; i < nc; i++)
		for (j = 0; j < nr; j++) {
			struct hint *hint = &hints[n++];

			hint->x = i * colgap + hint_size - w / 2;
			hint->y = j * rowgap + hint_size - h / 2;

			hint->w = w;
			hint->h = h;

			hint->label[0] = chars[i];
			hint->label[1] = chars[j];
			hint->label[2] = 0;
		}

	return n;
}

void init_hint_mode()
{
	init_hint(cfg->hint_bgcolor, cfg->hint_fgcolor, cfg->hint_border_radius,
		  cfg->hint_font);
}

static char code_to_char(uint8_t code)
{
	const char *s = input_lookup_name(code);

	if (!s)
		return 0;
	if (strlen(s) == 1)
		return s[0];
	if (!strcmp(s, "comma"))
		return ',';
	if (!strcmp(s, "dot"))
		return '.';
	if (!strcmp(s, "slash"))
		return '/';
	if (!strcmp(s, "semicolon"))
		return ';';

	return 0;
}

int hint_mode()
{
	screen_t scr;

	mouse_get_position(&scr, NULL, NULL);
	nr_hints = generate_hints(scr, hints);

	filter("");
	hint_draw(scr, matched, nr_matched);

	int  rc = 0;
	char buf[32] = {0};
	input_grab_keyboard();

	mouse_hide();

	while (1) {
		struct input_event *ev;
		ssize_t		    len;

		ev = input_next_event(0);

		if (!ev->pressed)
			continue;

		len = strlen(buf);

		if (input_event_eq(ev, cfg->exit)) {
			rc = -1;
			break;
		} else if (input_event_eq(ev, "C-u")) {
			buf[0] = 0;
		} else if (input_event_eq(ev, "backspace")) {
			if (len)
				buf[len - 1] = 0;
		} else {
			char c = code_to_char(ev->code);
			if (c)
				buf[len++] = c;
		}

		filter(buf);
		platform_commit();

		if (nr_matched == 1) {
			struct hint *h = &matched[0];
			mouse_move(scr, h->x + h->w / 2, h->y + h->h / 2);
			break;
		} else if (nr_matched == 0) {
			break;
		} else
			hint_draw(scr, matched, nr_matched);
	}

	input_ungrab_keyboard();
	hint_hide(scr);
	mouse_show();

	return rc;
}
