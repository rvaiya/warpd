/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

/*
 * DISCLAIMER: This was a small one off c file that ballooned into a small
 * project, I did not originally plan to publish it. Consequently the code is
 * ugly/will eat your face.
 *
 * I have subsequently tried to abstract away most of the platform specific
 * ugliness with mixed results. Bits of the code may still make
 * baby Jesus cry out in terror.
 *
 * You have been warned.
 */

#ifndef WARPD_H
#define WARPD_H

#ifndef __APPLE__
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#endif
#include "platform.h"

#include <getopt.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <time.h>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

enum {
	MODE_RESERVED,

	MODE_HINT,
	MODE_GRID,
	MODE_NORMAL,
	MODE_SCREEN_SELECTION,
};

struct config_entry {
	const char *key;
	const char *value;

	struct config_entry *next;
};

int hint_mode();
void screen_selection_mode();
struct input_event *normal_mode(struct input_event *start_ev);
struct input_event *grid_mode();

void init_hint_mode();
void init_normal_mode();
void init_grid_mode();

const char *input_event_tostr(struct input_event *ev);
int config_input_match(struct input_event *ev, const char *str, int strict);
int input_eq(struct input_event *ev, const char *str, int strict);
int config_input_match_index(struct input_event *ev, const char *config_name);
int input_parse_string(struct input_event *ev, const char *s);

void toggle_drag();

int mouse_process_key(struct input_event *ev, const char *up_key,
		      const char *down_key, const char *left_key,
		      const char *right_key);

void mouse_reset();
void mouse_fast();
void mouse_normal();
void mouse_slow();

void scroll_tick();
void scroll_stop();
void scroll_accelerate(int direction);
void scroll_decelerate();

int hist_add(int x, int y);
int hist_get(int *x, int *y);
void hist_prev();
void hist_next();
void init_mouse();

void parse_config(const char *path);
const char *config_get(const char *key);
int config_get_int(const char *key);
void config_print_options();

extern struct config_entry *config;
#endif
