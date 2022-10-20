/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stdlib.h>

#define MOD_CONTROL 1
#define MOD_SHIFT 2
#define MOD_META 4
#define MOD_ALT 8

#define SCROLL_DOWN 1
#define SCROLL_RIGHT 2
#define SCROLL_LEFT 3
#define SCROLL_UP 4

#define MAX_HINTS 2048
#define MAX_SCREENS 32

struct input_event {
	uint8_t code;
	uint8_t mods;
	uint8_t pressed;
};

struct hint {
	int x;
	int y;

	int w;
	int h;

	char label[16];
};

struct screen;
typedef struct screen *screen_t;

extern struct platform {
	/* Main entry point for each platform. Must be called befor other functions are
	 * usable. */

	void (*run)(void (*init)(void));

	/* Input */

	void (*input_grab_keyboard)();
	void (*input_ungrab_keyboard)();

	struct input_event *(*input_next_event)(int timeout);
	uint8_t (*input_lookup_code)(const char *name, int *shifted);
	const char *(*input_lookup_name)(uint8_t code, int shifted);

	/*
	 * Efficiently listen for one or more input events before
	 * grabbing the keyboard (including the event itself)
	 * and returning the matched event.
	 */
	struct input_event *(*input_wait)(struct input_event *events, size_t sz);

	void (*mouse_move)(screen_t scr, int x, int y);
	void (*mouse_down)(int btn);

	void (*mouse_up)(int btn);
	void (*mouse_click)(int btn);

	void (*mouse_get_position)(screen_t *scr, int *x, int *y);
	void (*mouse_show)();
	void (*mouse_hide)();

	void (*screen_get_dimensions)(screen_t scr, int *w, int *h);
	void (*screen_draw_box)(screen_t scr, int x, int y, int w, int h, const char *color);
	void (*screen_clear)(screen_t scr);
	void (*screen_list)(screen_t scr[MAX_SCREENS], size_t *n);

	void (*init_hint)(const char *bg, const char *fg, int border_radius, const char *font_family);

	/* Hints are centered around the provided x,y coordinates. */
	void (*hint_draw)(struct screen *scr, struct hint *hints, size_t n);

	void (*scroll)(int direction);

	void (*copy_selection)();

	/*
	* Draw operations may (or may not) be queued until this function
	* is called.
	*/
	void (*commit)();
} platform;

void platform_init();
#endif
