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
#define MAX_HIST_ENTS 16

enum {
	MODE_RESERVED,

	MODE_HISTORY,
	MODE_HINT,
	MODE_HINT2,
	MODE_GRID,
	MODE_NORMAL,
	MODE_HINTSPEC,
	MODE_SCREEN_SELECTION,
};

enum option_type {
	OPT_STRING,
	OPT_INT,

	OPT_KEY,
	OPT_BUTTON,
};

struct config_entry {
	const char *key;
	const char *value;
	enum option_type type;

	int whitelisted;

	struct config_entry *next;
};

struct histfile_ent {
	int x;
	int y;
};

extern char last_selected_hint[32];

int hintspec_mode();
int history_hint_mode();
int full_hint_mode(int second_pass);
void screen_selection_mode();
struct input_event *normal_mode(struct input_event *start_ev, int oneshot);
struct input_event *grid_mode();

void init_hints();
void init_normal_mode();
void init_grid_mode();

void config_input_whitelist(const char *names[], size_t n);

const char *input_event_tostr(struct input_event *ev);
int input_eq(struct input_event *ev, const char *str);
int input_parse_string(struct input_event *ev, const char *s);
int config_input_match(struct input_event *ev, const char *str);

size_t hist_hints(struct hint *hints, int w, int h);

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

void hist_add(int x, int y);
int hist_get(int *x, int *y);
void hist_prev();
void hist_next();


size_t histfile_read(struct histfile_ent **entries);
void histfile_add(int x, int y);

void init_mouse();

const char *get_config_path(const char *file);
const char *get_data_path(const char *file);
void parse_config(const char *path);
const char *config_get(const char *key);
int config_get_int(const char *key);
void config_print_options();

extern struct config_entry *config;
#endif
