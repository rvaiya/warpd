/*
 * warpd - A keyboard-driven modal pointer.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#ifndef WARPD
#define WARPD

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include "cfg.h"
#include "platform.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

extern struct cfg *cfg;

enum {
	MODE_RESERVED,

	MODE_HINT,
	MODE_GRID,
	MODE_NORMAL,
};

void screen_get_dimensions(int *x, int *y);

struct input_event *normal_mode(struct input_event *start_ev);
struct input_event *grid_mode();
int		    hint_mode();

void init_hint_mode();
void init_normal_mode();
void init_grid_mode();

const char *input_event_tostr(struct input_event *ev);
int	    input_event_eq(struct input_event *ev, const char *str);
int	    input_parse_string(struct input_event *ev, const char *s);

void toggle_drag();

int mouse_process_key(struct input_event *ev, const char *up_key,
		      const char *down_key, const char *left_key,
		      const char *right_key);

void mouse_reset();

void scroll_tick();
void scroll_stop();
void scroll_accelerate(int direction);
void scroll_decelerate();

int  hist_add(int x, int y);
int  hist_get(int *x, int *y);
void hist_prev();
void hist_next();
void init_mouse();

#endif
