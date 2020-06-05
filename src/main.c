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
#include <limits.h>
#include <unistd.h>
#include "grid.h"
#include "hints.h"
#include "discrete.h"
#include "cfg.h"
#include "dbg.h"
#include "input.h"

#define MAX_BTNS 8

static Display *dpy;
static int opt_daemonize = 0;

static const char usage[] = 
"warp [-l] [-h] [-v] \n\n"
"See the manpage for more details and usage examples.\n";
static struct cfg *cfg;
static uint16_t drag_key = 0;

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
	if(!fork())
		setsid();
	else
		exit(0);

	printf("Daemon started\n");
	freopen("/dev/null", "w", stderr);
	freopen("/dev/null", "w", stdout);
}

static void proc_args(char **argv, int argc) 
{
	int opt;

	while((opt = getopt(argc, argv, "dlv")) != -1) {
		switch(opt) {
			size_t i;
		case 'l':
			for(i = 0; i < 256; i++) {
				const char *s = XKeysymToString(XKeycodeToKeysym(dpy, i, 0));
				if(s) printf("%s\n", s);
			}
			exit(0);
			break;
		case 'd':
			opt_daemonize++;
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

	sprintf(lock_file, "%s/warp.lock.%s", rundir, getenv("DISPLAY"));

	if((fd = open(lock_file, O_CREAT | O_TRUNC, 0600)) < 0) {
		perror("open");
		exit(1);
	}

	if(flock(fd, LOCK_EX | LOCK_NB)) {
		fprintf(stderr, "ERROR: warp already appears to be running.\n");
		exit(1);
	}
}

uint16_t query_intent(uint16_t keyseq)
{
	static uint16_t discrete_activation_key = 0;
	static uint16_t hint_activation_key;
	static uint16_t grid_activation_key;

	if(!discrete_activation_key) {
		discrete_activation_key = get_keyseq(cfg->discrete_activation_key);
		hint_activation_key = get_keyseq(cfg->hint_activation_key);
		grid_activation_key = get_keyseq(cfg->grid_activation_key);
	}

	if(grid_activation_key == keyseq)
		return query_intent(grid_warp(-1,-1));
	else if(hint_activation_key == keyseq) {
		if((keyseq=hint_warp()))
			return query_intent(keyseq);
		else
			return query_intent(discrete_warp(0));
	} else if(discrete_activation_key == keyseq)
		return query_intent(discrete_warp(0));

	return keyseq;
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

int main(int argc, char **argv) 
{
	size_t i;
	char path[PATH_MAX];

	struct hint_keys hint_keys;
	struct grid_keys grid_keys;
	struct discrete_keys discrete_keys;

	uint16_t buttons[MAX_BTNS];
	uint16_t exit_keys[MAX_EXIT_KEYS] = {0};

	uint16_t discrete_activation_key;
	uint16_t hint_activation_key;
	uint16_t grid_activation_key;

	if(!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr,
			"Failed to establish X connection, make sure X is running.\n");
		return -1;
	}

	proc_args(argv, argc);
	check_lock_file();

	sprintf(path, "%s/.warprc", getenv("HOME"));
	cfg = parse_cfg(path);
	init_input(dpy);

	hint_keys = (struct hint_keys){
		up:     get_keyseq(cfg->hint_up),
		down:   get_keyseq(cfg->hint_down),
		left:   get_keyseq(cfg->hint_left),
		right:  get_keyseq(cfg->hint_right),
	};

	discrete_keys = (struct discrete_keys){
		up:        get_keyseq(cfg->discrete_up),
		down:      get_keyseq(cfg->discrete_down),
		left:      get_keyseq(cfg->discrete_left),
		right:     get_keyseq(cfg->discrete_right),

		home:      get_keyseq(cfg->discrete_home),
		middle:    get_keyseq(cfg->discrete_middle),
		last:      get_keyseq(cfg->discrete_last),
		beginning: get_keyseq(cfg->discrete_beginning),
		end:       get_keyseq(cfg->discrete_end),
	};

	grid_keys = (struct grid_keys){
		up:    get_keyseq(cfg->grid_up),
		down:  get_keyseq(cfg->grid_down),
		left:  get_keyseq(cfg->grid_left),
		right: get_keyseq(cfg->grid_right),
	};

	dbg("Built from commit: "COMMIT"\n");
	parse_keylist(cfg->buttons,
		      buttons,
		      sizeof buttons / sizeof buttons[0]);

	parse_keylist(cfg->grid_keys,
		      grid_keys.grid,
		      sizeof grid_keys.grid / sizeof grid_keys.grid[0]);

	discrete_activation_key = get_keyseq(cfg->discrete_activation_key);
	hint_activation_key = get_keyseq(cfg->hint_activation_key);
	grid_activation_key = get_keyseq(cfg->grid_activation_key);
	drag_key = get_keyseq(cfg->drag_key);

	for (i = 0; i < sizeof buttons / sizeof buttons[0]; i++)
		exit_keys[i] = buttons[i]; 

	exit_keys[i++] = get_keyseq(cfg->exit);
	exit_keys[i++] = discrete_activation_key;
	exit_keys[i++] = hint_activation_key;
	exit_keys[i++] = grid_activation_key;
	exit_keys[i++] = drag_key;

	memcpy(discrete_keys.exit, exit_keys, sizeof discrete_keys.exit);
	memcpy(grid_keys.exit, exit_keys, sizeof grid_keys.exit);

	if(opt_daemonize) daemonize();

	init_hint(dpy,
		  cfg->hint_characters,
		  cfg->hint_bgcol,
		  cfg->hint_fgcol,
		  cfg->hint_width,
		  cfg->hint_height,
		  cfg->hint_opacity,
		  &hint_keys);

	init_grid(dpy,
		  cfg->grid_nr,
		  cfg->grid_nc,
		  cfg->grid_line_width,
		  cfg->grid_pointer_size,
		  cfg->movement_increment,
		  cfg->grid_col,
		  cfg->grid_mouse_col,
		  &grid_keys);

	init_discrete(dpy,
		      cfg->movement_increment,
		      &discrete_keys,
		      cfg->discrete_color,
		      cfg->discrete_size);

	while(1) {
		uint16_t grabbed_keys[] = {
			grid_activation_key,
			hint_activation_key,
			discrete_activation_key
		};

		uint16_t activation_key;
		uint16_t intent_key;

		set_cursor_visibility(1);
		activation_key = input_wait_for_key(grabbed_keys, sizeof grabbed_keys / sizeof grabbed_keys[0]);
		set_cursor_visibility(0);
start:
		input_grab_keyboard();

		intent_key = query_intent(activation_key);

		if(intent_key == drag_key) {
			XTestFakeButtonEvent(dpy, 1, True, CurrentTime);
			query_intent(activation_key);
			XTestFakeButtonEvent(dpy, 1, False, CurrentTime);
			input_ungrab_keyboard();
			continue;
		}

		size_t btn = 0;
		while(btn < MAX_BTNS) {
			if(buttons[btn] == (intent_key & 0xFF)) {
				btn++;
				break;
			}
			btn++;
		}

		switch(btn) {
		case 1:
			set_cursor_visibility(1);
			input_click(1);
			while(input_next_key(cfg->double_click_timeout) == buttons[0]) {
				input_click(1);
			}
			break;
		case 2:
			input_click(2);
			break;
		case 3:
			input_click(3);
			break;
		case 4:
		case 5:
			{
				const int v0 = 10; //scroll events per second
				const int a = 30;

				int t = 0; //in ms
				int v = v0;
				int last_click = 0;

				XTestFakeButtonEvent(dpy, btn, True, CurrentTime);
				XTestFakeButtonEvent(dpy, btn, False, CurrentTime);

				while(1) {
					if(input_next_keyup(1) != TIMEOUT_KEYSEQ)
						break;

					t += 1;
					if((t - last_click)*v > 1000) {
						XTestFakeButtonEvent(dpy, btn, True, CurrentTime);
						XTestFakeButtonEvent(dpy, btn, False, CurrentTime);
						last_click = t;
					}
					v = (a*t)/1000 + v0;
				}

				activation_key = discrete_activation_key;
				goto start;
			}

			break;
		}

		input_ungrab_keyboard();
	}
}
