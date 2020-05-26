/*
 * ---------------------------------------------------------------------
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 * Original Author: Raheman Vaiya
 * ---------------------------------------------------------------------
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/file.h>
#include <signal.h>
#include <limits.h>

#include "keynames.h"
#include "grid.h"
#include "hints.h"
#include "discrete.h"
#include "cfg.h"
#include "dbg.h"

const char usage[] = 
"warp [-l] [-h] \n\n"
"See the manpage for more details and usage examples.\n";

char lock_file[PATH_MAX];
int opt_daemonize = 0;
Display *dpy;

static int parse_mods(const char* s) 
{
	int mods = 0;
	if(!s) return 0;
	while(*s) {
		switch(s[0]) {
		case 'A':
			mods |= Mod1Mask;
			break;
		case 'M':
			mods |= Mod4Mask;
			break;
		case 'S':
			mods |= ShiftMask;
			break;
		case 'C':
			mods |= ControlMask;
			break;
		default:
			fprintf(stderr, "%c is not a valid modifier\n", s[0]);
			exit(1);
		}

		s++;
		if(s[0] == '-') s++;
	}

	return mods;
}

static void parse_keyseq(const char* key, int *code, int *mods) 
{
	KeySym sym;
	if(!key || key[0] == 0) return;

	*code = 0;
	*mods = 0;

	while(key[1] == '-') {
		switch(key[0]) {
		case 'A':
			*mods |= Mod1Mask;
			break;
		case 'M':
			*mods |= Mod4Mask;
			break;
		case 'S':
			*mods |= ShiftMask;
			break;
		case 'C':
			*mods |= ControlMask;
			break;
		default:
			fprintf(stderr, "%s is not a valid key\n", key);
			exit(1);
		}

		key += 2;
	}

	if(key[0]) {
		sym = XStringToKeysym(key);

		if(sym == NoSymbol) {
			fprintf(stderr, "Could not find keysym for %s\n", key);
			exit(1);
		}

		if(key[1] == '\0' && isupper(key[0]))
			*mods |= ShiftMask;

		*code = XKeysymToKeycode(dpy, sym);
	}
}

KeyCode keycode(const char *s) 
{
	KeySym sym;

	if(!(sym = XStringToKeysym(s))) {
		fprintf(stderr, "ERROR: \"%s\" is not a valid keysym\n", s);
		exit(-1);
	}

	return XKeysymToKeycode(dpy, sym);
}

int parse_keylist(const char *s, KeyCode *keycodes, int sz)
{
	int i, n = 0;
	char *_s = strdup(s), *key;


	key = strtok(_s, ",");
	for(i=0; i < sz; i++) {
		if(key) {
			KeySym sym;

			if(!(sym = XStringToKeysym(key))) {
				fprintf(stderr, "ERROR: \"%s\" is not a valid key\n", key);
				exit(-1);
			}

			keycodes[n++] = XKeysymToKeycode(dpy, sym);
		} else
			keycodes[i] = 0;

		key = strtok(NULL, ",");
	}

	free(_s);
	return n;
}

static void grab_keyseq(const char *keyseq) 
{
	if(!keyseq) return;

	int code, mods;

	parse_keyseq(keyseq, &code, &mods);

	XGrabKey(dpy,
		 code,
		 mods,
		 DefaultRootWindow(dpy),
		 False,
		 GrabModeAsync,
		 GrabModeAsync);

	XGrabKey(dpy,
		 code,
		 mods | Mod2Mask, //To avoid being foiled by numlock.
		 DefaultRootWindow(dpy),
		 False,
		 GrabModeAsync,
		 GrabModeAsync);
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
	int r, c;
	int opt;

	while((opt = getopt(argc, argv, "dl")) != -1) {
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
		default:
			fprintf(stderr, usage);
			exit(1);
		}
	}
}

static void check_lock_file() 
{
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

static int keyseq_eq(const char *s, XKeyEvent *ev) 
{
	if(!s) return 0;

	int code, mods;
	parse_keyseq(s, &code, &mods);
	return (ev->state & ~Mod2Mask) == mods && ev->keycode == code;
}

static int loop(struct cfg *cfg)
{
	int trigger_mods = parse_mods(cfg->trigger_mods);
	char *s;
	int i,j;
	struct hint_keys hint_keys;
	struct grid_keys grid_keys;
	struct discrete_keys discrete_keys;

	hint_keys = (struct hint_keys){
		up:     keycode(cfg->hint_up),
		down:   keycode(cfg->hint_down),
		left:   keycode(cfg->hint_left),
		right:  keycode(cfg->hint_right),
		quit:   keycode(cfg->close_key),
	};

	discrete_keys = (struct discrete_keys){
		up:     keycode(cfg->discrete_up),
		down:   keycode(cfg->discrete_down),
		left:   keycode(cfg->discrete_left),
		right:  keycode(cfg->discrete_right),
		quit:   keycode(cfg->close_key),
	};

	grid_keys = (struct grid_keys){
		up:         keycode(cfg->grid_up),
		down:       keycode(cfg->grid_down),
		left:       keycode(cfg->grid_left),
		right:      keycode(cfg->grid_right),
		close_key:  keycode(cfg->close_key),
	};

	parse_keylist(cfg->buttons, discrete_keys.buttons, sizeof discrete_keys.buttons);
	parse_keylist(cfg->buttons, grid_keys.buttons, sizeof grid_keys.buttons);
	parse_keylist(cfg->buttons, hint_keys.buttons, sizeof hint_keys.buttons);

	if(parse_keylist(cfg->grid_keys,
		grid_keys.grid,
		sizeof grid_keys.grid) < (cfg->grid_nc * cfg->grid_nr)) {
		fprintf(stderr, "ERROR: Insufficient number of keys supplied to grid_key argument (must include %d keys).\n", cfg->grid_nc*cfg->grid_nr);
		return -1;
	}

	grab_keyseq(cfg->hint_activation_key);
	grab_keyseq(cfg->grid_activation_key);
	grab_keyseq(cfg->discrete_activation_key);

	if(trigger_mods) {
		for(int i = 0;i<cfg->grid_nr;i++)
			for(int j = 0;j<cfg->grid_nc;j++) {
				XGrabKey(dpy,
					 grid_keys.grid[i*cfg->grid_nc+j],
					 trigger_mods,
					 DefaultRootWindow(dpy),
					 False,
					 GrabModeAsync,
					 GrabModeAsync);

				XGrabKey(dpy,
					 grid_keys.grid[i*cfg->grid_nc+j],
					 trigger_mods | Mod2Mask,
					 DefaultRootWindow(dpy),
					 False,
					 GrabModeAsync,
					 GrabModeAsync);
			}
	}

	dbg("Entering main loop.");
	while(1) {
		XEvent ev;
		XNextEvent(dpy, &ev);

		if(ev.type == KeyPress) {
			if(trigger_mods && (ev.xkey.state & ~Mod2Mask) == trigger_mods) {
				int found = 0;

				for(int i = 0;i<cfg->grid_nr && !found;i++)
					for(int j = 0;j<cfg->grid_nc && !found;j++)
						if(ev.xkey.keycode == grid_keys.grid[i*cfg->grid_nc+j]) {
							grid(dpy,
							     cfg->grid_nr,
							     cfg->grid_nc,
							     cfg->grid_line_width,
							     cfg->grid_pointer_size,
							     cfg->grid_activation_timeout,
							     cfg->movement_increment,
							     i, j,
							     cfg->grid_col,
							     cfg->grid_mouse_col,
							     &grid_keys);

							found++;
						}
			} else if(keyseq_eq(cfg->grid_activation_key, &ev.xkey)) {
				grid(dpy,
				     cfg->grid_nr,
				     cfg->grid_nc,
				     cfg->grid_line_width,
				     cfg->grid_pointer_size,
				     cfg->grid_activation_timeout,
				     cfg->movement_increment,
				     -1,
				     -1,
				     cfg->grid_col,
				     cfg->grid_mouse_col,
				     &grid_keys);

			} else if(keyseq_eq(cfg->discrete_activation_key, &ev.xkey)) {
				discrete(dpy,
					 cfg->movement_increment,
					 cfg->grid_activation_timeout,
					 &discrete_keys,
					 cfg->discrete_indicator_color,
					 cfg->discrete_indicator_size);
			} else {
				hints(dpy,
				      cfg->hint_nc,
				      cfg->hint_nr,
				      cfg->hint_characters,
				      cfg->movement_increment,
				      cfg->hint_bgcol,
				      cfg->hint_fgcol,
				      &hint_keys);
			}
		}
	}
}

int main(int argc, char **argv) 
{
	char path[PATH_MAX];
	struct cfg *cfg;

	if(!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr,
			"Failed to establish X connection, make sure X is running.\n");
		return -1;
	}

	sprintf(path, "%s/.warprc", getenv("HOME"));
	cfg = parse_cfg(path);
	proc_args(argv, argc);
	check_lock_file();

	if(opt_daemonize) daemonize();

	return loop(cfg);
}
