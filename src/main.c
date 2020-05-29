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
#include <sys/file.h>
#include <limits.h>
#include <unistd.h>
#include "grid.h"
#include "hints.h"
#include "discrete.h"
#include "cfg.h"
#include "dbg.h"
#include "input.h"
#include "keynames.h"

static Display *dpy;
static int opt_daemonize = 0;

static const char usage[] = 
"warp [-l] [-h] [-v] \n\n"
"See the manpage for more details and usage examples.\n";

static uint16_t keyseq(const char *s) 
{
	uint16_t seq;

	seq = input_parse_keyseq(s);
	if(!seq) {
		fprintf(stderr, "ERROR: \"%s\" is not a valid key key sequence\n", s);
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
			for(i = 0; i < sizeof(keynames)/sizeof(keynames[0]); i++)
				printf("%s\n", keynames[i]);
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

int main(int argc, char **argv) {
	char path[PATH_MAX];
	struct cfg *cfg;

	struct hint_keys hint_keys;
	struct grid_keys grid_keys;
	struct discrete_keys discrete_keys;

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
		up:     keyseq(cfg->hint_up),
		down:   keyseq(cfg->hint_down),
		left:   keyseq(cfg->hint_left),
		right:  keyseq(cfg->hint_right),
		quit:   keyseq(cfg->close_key),
	};

	discrete_keys = (struct discrete_keys){
		up:     keyseq(cfg->discrete_up),
		down:   keyseq(cfg->discrete_down),
		left:   keyseq(cfg->discrete_left),
		right:  keyseq(cfg->discrete_right),
		quit:   keyseq(cfg->close_key),
	};

	grid_keys = (struct grid_keys){
		up:         keyseq(cfg->grid_up),
		down:       keyseq(cfg->grid_down),
		left:       keyseq(cfg->grid_left),
		right:      keyseq(cfg->grid_right),
		close_key:  keyseq(cfg->close_key),
	};

	parse_keylist(cfg->buttons, discrete_keys.buttons, sizeof discrete_keys.buttons / sizeof discrete_keys.buttons[0]);
	parse_keylist(cfg->buttons, grid_keys.buttons, sizeof grid_keys.buttons / sizeof grid_keys.buttons[0]);

	parse_keylist(cfg->grid_keys,
		      grid_keys.grid,
		      sizeof grid_keys.grid / sizeof grid_keys.grid[0]);

	uint16_t discrete_activation_key = keyseq(cfg->discrete_activation_key);
	uint16_t hint_activation_key = keyseq(cfg->hint_activation_key);
	uint16_t grid_activation_key = keyseq(cfg->grid_activation_key);

	uint16_t grab_keys[] = {
		grid_activation_key,
		hint_activation_key,
		discrete_activation_key,
	};

	if(opt_daemonize) daemonize();
	while(1) {
		int start_btn = 0;

		uint16_t keyseq = input_wait_for_key(grab_keys, sizeof grab_keys/sizeof grab_keys[0]);

		if(grid_activation_key == keyseq) {
			start_btn = grid(dpy,
					 cfg->grid_nr,
					 cfg->grid_nc,
					 cfg->grid_line_width,
					 cfg->grid_pointer_size,
					 cfg->movement_increment,
					 -1, -1,
					 cfg->grid_col,
					 cfg->grid_mouse_col,
					 &grid_keys);
		} else if(hint_activation_key == keyseq) {
			start_btn = hints(dpy,
					  cfg->hint_nc,
					  cfg->hint_nr,
					  cfg->hint_characters,
					  cfg->hint_bgcol,
					  cfg->hint_fgcol,
					  &hint_keys);
		}

		if(start_btn != -1) {
			discrete(dpy,
				 cfg->movement_increment,
				 cfg->double_click_timeout,
				 start_btn,
				 &discrete_keys,
				 cfg->discrete_indicator_color,
				 cfg->discrete_indicator_size);
		}

		input_ungrab_keyboard();
	}
}
