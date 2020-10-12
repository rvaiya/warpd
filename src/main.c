/* Copyright © 2019 Raheman Vaiya.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <X11/keysym.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XTest.h>
#include <sys/file.h>
#include <signal.h>
#include <limits.h>
#include <unistd.h>
#include "grid.h"
#include "hints.h"
#include "normal.h"
#include "cfg.h"
#include "dbg.h"
#include "input.h"
#include "scroll.h"

#define MAX_BTNS 8

#define NORMAL_MODE 0
#define HINT_MODE 1
#define GRID_MODE 2

static Display *dpy;
static int opt_foreground = 0;

static const char usage[] = 
"warpd [-l] [-h] [-v] \n\n"
"See the manpage for more details and usage examples.\n";
static struct cfg *cfg;

static uint16_t get_keyseq(const char *s) 
{
	uint16_t seq;

	seq = input_parse_keyseq(s);
	if(!seq) {
		fprintf(stderr, "ERROR: \"%s\" is not a valid key sequence\n", s);
		exit(-1);
	}

	return seq;
}

static int parse_keylist(const char *s, uint16_t *keycodes, int sz)
{
	int i, n = 0;
	char *_s = strdup(s), *key;


	key = strtok(_s, ",");
	for(i=0; i < sz; i++) {
		if(key) {
			uint16_t seq = input_parse_keyseq(key);
			if(!seq) {
				fprintf(stderr, "ERROR: \"%s\" is not a valid key sequence\n", key);
				exit(-1);
			}
			keycodes[i] = seq;
			n = i+1;
		} else
			keycodes[i] = 0;

		key = strtok(NULL, ",");
	}

	free(_s);
	return n;
}

static void daemonize() 
{
	const char *path;

	if(!fork())
		setsid();
	else
		exit(0);

	if((path=getenv("WARP_DEBUG_FILE"))) {
		if(!freopen(path, "w", stderr)) {
			perror("freopen");
			exit(-1);
		}
		init_dbg();
	} else
		freopen("/dev/null", "w", stderr);


	printf("Daemon started\n");
	freopen("/dev/null", "w", stdout);
}

static void proc_args(char **argv, int argc) 
{
	int opt;

	while((opt = getopt(argc, argv, "flv")) != -1) {
		switch(opt) {
			size_t i;
		case 'l':
			for(i = 0; i < 256; i++) {
				const char *s = XKeysymToString(XKeycodeToKeysym(dpy, i, 0));
				if(s) printf("%s\n", s);
			}
			exit(0);
			break;
		case 'f':
			opt_foreground++;
			break;
		case 'v':
			fprintf(stderr, "Compiled from git commit: "COMMIT"\n");
			exit(0);
			break;
		default:
			fprintf(stderr, usage);
			exit(1);
		}
	}
}

static void check_lock_file() 
{
	char lock_file[PATH_MAX];
	int fd;

	const char *rundir = getenv("XDG_RUNTIME_DIR");
	if(!rundir) {
		fprintf(stderr, "Could not find XDG_RUNTIME_DIR, make sure X is running.");
		exit(1);
	}

	sprintf(lock_file, "%s/warpd.lock.%s", rundir, getenv("DISPLAY"));

	if((fd = open(lock_file, O_CREAT | O_TRUNC, 0600)) < 0) {
		perror("open");
		exit(1);
	}

	if(flock(fd, LOCK_EX | LOCK_NB)) {
		fprintf(stderr, "ERROR: warpd already appears to be running.\n");
		exit(1);
	}
}

//XFixes* functions are not idempotent (calling them more than
//once crashes the client, so we need this wrapper function).

static void set_cursor_visibility(int visible) 
{
	static int state = 1;

	if(visible == state) return;

	if(visible)
		XFixesShowCursor(dpy, DefaultRootWindow(dpy));
	else
		XFixesHideCursor(dpy, DefaultRootWindow(dpy));

	XFlush(dpy);
	state = visible;
}

static void grid_init(uint16_t *exit_keys, size_t n)
{
	static struct grid_keys keys;

	keys = (struct grid_keys) {
		up: get_keyseq(cfg->grid_up),
		down: get_keyseq(cfg->grid_down),
		left: get_keyseq(cfg->grid_left),
		right: get_keyseq(cfg->grid_right)
	};

	parse_keylist(cfg->grid_keys,
		      keys.grid,
		      sizeof keys.grid / sizeof keys.grid[0]);

	memcpy(keys.exit, exit_keys, n * sizeof(exit_keys[0]));

	init_grid(dpy,
		  cfg->grid_nr,
		  cfg->grid_nc,
		  cfg->grid_line_width,
		  cfg->grid_pointer_size,
		  cfg->movement_increment,
		  cfg->grid_color,
		  cfg->grid_mouse_color,
		  &keys);
}

static void hint_init()
{
	init_hint(dpy,
		  cfg->hint_characters,
		  cfg->hint_bgcolor,
		  cfg->hint_fgcolor,
		  cfg->hint_width,
		  cfg->hint_height,
		  cfg->hint_opacity);
}

static void normal_init(uint16_t *exit_keys,
	size_t n,
	uint16_t scroll_up,
	uint16_t scroll_down,
	uint16_t scroll_left,
	uint16_t scroll_right)
{
	static struct normal_keys keys;

	keys = (struct normal_keys) {
		up: get_keyseq(cfg->normal_up),
		down: get_keyseq(cfg->normal_down),
		left: get_keyseq(cfg->normal_left),
		right: get_keyseq(cfg->normal_right),

		right_word: get_keyseq(cfg->normal_right_word),
		left_word: get_keyseq(cfg->normal_left_word),
		up_word: get_keyseq(cfg->normal_up_word),
		down_word: get_keyseq(cfg->normal_down_word),

		home: get_keyseq(cfg->normal_home),
		middle: get_keyseq(cfg->normal_middle),
		last: get_keyseq(cfg->normal_last),
		beginning: get_keyseq(cfg->normal_beginning),
		end: get_keyseq(cfg->normal_end),

		scroll_up: scroll_up,
		scroll_down: scroll_down,
		scroll_left: scroll_left,
		scroll_right: scroll_right,
	};

	memcpy(keys.exit, exit_keys, sizeof exit_keys[0] * n);

	init_normal(dpy,
		    cfg->movement_increment,
		    cfg->normal_word_size,
		    &keys,
		    cfg->normal_color,
		    cfg->normal_size,
		    cfg->scroll_fling_timeout,
		    cfg->scroll_velocity,
		    cfg->scroll_acceleration,
		    cfg->scroll_fling_velocity,
		    cfg->scroll_fling_acceleration,
		    cfg->scroll_fling_deceleration);
}

uint16_t get_action(int start_mode, uint16_t grid_key, uint16_t hint_key, uint16_t exit_key)
{
	uint16_t key;

	if(start_mode == GRID_MODE) return grid_mode(-1, -1);
	if(start_mode == HINT_MODE && hint_mode() == exit_key) return exit_key;

	while(1) {
		key = normal_mode(0);

		if(key == hint_key)
			hint_mode();
		else if(key == grid_key) {
			if((key = grid_mode(-1, -1)) != exit_key)
				return key;
		} else
			return key;
	}
}

void main_loop()
{
	size_t i, n;

	uint16_t buttons[MAX_BTNS];
	uint16_t exit_keys[MAX_EXIT_KEYS] = {0};

	uint16_t normal_key = get_keyseq(cfg->normal_activation_key);
	uint16_t hint_key = get_keyseq(cfg->hint_activation_key);
	uint16_t grid_key = get_keyseq(cfg->grid_activation_key);

	uint16_t scroll_up_key = get_keyseq(cfg->scroll_up_key);
	uint16_t scroll_down_key = get_keyseq(cfg->scroll_down_key);
	uint16_t scroll_left_key = get_keyseq(cfg->scroll_left_key);
	uint16_t scroll_right_key = get_keyseq(cfg->scroll_right_key);

	uint16_t normal_hint_key = get_keyseq(cfg->normal_hint_key);
	uint16_t normal_grid_key = get_keyseq(cfg->normal_grid_key);

	uint16_t drag_key = get_keyseq(cfg->drag_key);
	uint16_t exit_key = get_keyseq(cfg->exit);
	int oneshot = !strcmp(cfg->oneshot_mode, "true");

	parse_keylist(cfg->buttons,
		      buttons,
		      sizeof buttons / sizeof buttons[0]);

	n = 0;
	for (i = 0; i < 3; i++) {
		if(buttons[i]) {
			exit_keys[n++] = buttons[i]; 
			exit_keys[n++] = buttons[i] | (Mod1Mask << 8);
			exit_keys[n++] = buttons[i] | (Mod4Mask << 8);
			exit_keys[n++] = buttons[i] | (ShiftMask << 8);
			exit_keys[n++] = buttons[i] | (ControlMask << 8);
		}
	}

	exit_keys[n++] = exit_key;
	exit_keys[n++] = drag_key;

	hint_init();
	grid_init(exit_keys, n);

	exit_keys[n++] = normal_hint_key;
	exit_keys[n++] = normal_grid_key;
	normal_init(exit_keys, n, buttons[3], buttons[4], buttons[5], buttons[6]);

	while(1) {
		uint16_t grabbed_keys[] = {
			grid_key,
			hint_key,
			normal_key,

			scroll_up_key,
			scroll_down_key,
			scroll_left_key,
			scroll_right_key,
		};

		uint16_t key;

		dbg("Waiting for activation key");

		key = input_wait_for_key(grabbed_keys, sizeof grabbed_keys / sizeof grabbed_keys[0]);
//start:
		dbg("Processing activation key %s.", input_keyseq_to_string(key));
		input_grab_keyboard(1);
		set_cursor_visibility(0);

		if(key == scroll_left_key || key == scroll_right_key || key == scroll_up_key || key == scroll_down_key) {
			while(key == scroll_left_key || key == scroll_right_key || key == scroll_up_key || key == scroll_down_key) {
				key = scroll(dpy, key, 
					     key == scroll_down_key ? 5 : 
					     key == scroll_up_key ? 4 : 
					     key == scroll_left_key ? 6 : 7,
					     cfg->scroll_velocity,
					     cfg->scroll_acceleration,
					     cfg->scroll_fling_velocity,
					     cfg->scroll_fling_acceleration,
					     cfg->scroll_fling_deceleration,
					     cfg->scroll_fling_timeout);
			}
		} else {
			int mode = key == hint_key ? HINT_MODE :
				   key == grid_key ? GRID_MODE :
				   NORMAL_MODE;

			while(1) {
				uint16_t action = get_action(mode, normal_grid_key, normal_hint_key, exit_key);


				if(action == buttons[0])      input_click(1);
				else if(action == buttons[1]) input_click(2);
				else if(action == buttons[2]) input_click(3);
				else if(action == drag_key) {
					XTestFakeButtonEvent(dpy, 1, True, CurrentTime);
					get_action(NORMAL_MODE, normal_grid_key, normal_hint_key, exit_key);
					XTestFakeButtonEvent(dpy, 1, False, CurrentTime);
				}
				else break;

				if(oneshot) {
					while(action == buttons[0] && input_next_key(cfg->double_click_timeout, 0) == buttons[0])
						input_click(1);

					break;
				}

				mode = NORMAL_MODE;
			}
		}

		input_ungrab_keyboard(1);
		set_cursor_visibility(1);
	}
}

static void cleanup(int _)
{
	fprintf(stderr, "Received termination signal, cleaning up...\n");
	input_ungrab_keyboard(0);
	exit(1);
}

int main(int argc, char **argv) 
{
	char path[PATH_MAX];
	sprintf(path, "%s/.warprc", getenv("HOME"));
	cfg = parse_cfg(path);

	dbg("Built from commit: "COMMIT"\n");

	if(!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr,
			"Failed to establish X connection, make sure X is running.\n");
		return -1;
	}

	proc_args(argv, argc);
	check_lock_file();

	if(opt_foreground) 
		init_dbg();
	else 
		daemonize();

	signal(SIGINT, cleanup);
	signal(SIGTERM, cleanup);
	init_input(dpy);

	main_loop();
}
