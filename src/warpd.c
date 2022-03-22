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

#include "warpd.h"

#include <sys/stat.h>
#include <sys/file.h>
#include <stdio.h>
#include <fcntl.h>

struct cfg *cfg;
static int dragging = 0;
char config_dir[512];

void toggle_drag()
{
	dragging = !dragging;
	if (dragging)
		mouse_down(1);
	else
		mouse_up(1);
}

static void activation_loop(int mode)
{
	struct input_event *ev = NULL;

	dragging = 0;

	while (1) {
		switch (mode) {
			case MODE_NORMAL:
				ev = normal_mode(ev);

				if (input_event_eq(ev, cfg->hint))
					mode = MODE_HINT;
				else if (input_event_eq(ev, cfg->grid))
					mode = MODE_GRID;
				else if (input_event_eq(ev, cfg->exit) || !ev)
					goto exit;

				break;
			case MODE_HINT:
				if (hint_mode() < 0)
					goto exit;

				ev = NULL;
				mode = MODE_NORMAL;
				break;
			case MODE_GRID:
				ev = grid_mode();
				mode = MODE_NORMAL;
				break;
		}
	}

exit:
	if (dragging)
		toggle_drag();
	return;
}

static void main_loop()
{
	init_grid(cfg->grid_color, cfg->grid_size, cfg->grid_nc, cfg->grid_nr);
	init_normal_mode();
	init_hint_mode();

	struct input_event activation_events[4] = {0};

	input_parse_string(&activation_events[0], cfg->activation_key);
	input_parse_string(&activation_events[1], cfg->hint_activation_key);
	input_parse_string(&activation_events[2], cfg->grid_activation_key);
	input_parse_string(&activation_events[3], cfg->hint_oneshot_key);

	while(1) {
		int mode = 0;
		struct input_event *ev = input_wait(activation_events, 4);

		if (input_event_eq(ev, cfg->activation_key))
			mode = MODE_NORMAL;
		else if (input_event_eq(ev, cfg->grid_activation_key))
			mode = MODE_GRID;
		else if (input_event_eq(ev, cfg->hint_activation_key))
			mode = MODE_HINT;
		else if (input_event_eq(ev, cfg->hint_oneshot_key)) {
			hint_mode();
			continue;
		}

		activation_loop(mode);
	}
}

static void lock()
{
	char path[1024];
	int fd;
	sprintf(path, "%s/lock", config_dir);

	if ((fd = open(path, O_CREAT | O_RDWR, 0600)) == -1) {
		perror("flock open");
		exit(1);
	}

	if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
		fprintf(stderr, "ERROR: Another instance of warpd is already running.\n");
		exit(-1);
	}
}

static void daemonize()
{
	char path[1024];

	if (fork()) exit(0);
	if (fork()) exit(0);

	sprintf(path, "%s/warpd.log", config_dir);
	printf("daemonizing, log output stored in %s.\n", path);

	int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		perror("open");
		exit(-1);

	}

	close(1);
	close(2);

	dup2(fd, 1);
	dup2(fd, 2);
}

static void print_keys()
{
	/* TODO: implement */
}

static void print_version()
{
	printf("warpd v"VERSION" (built from: "COMMIT")\n");
}

int main(int argc, char *argv[])
{
	int foreground_flag = 0;
	char config_path[1024];
	const char *home = getenv("HOME");

	if (!home) {
		fprintf(stderr, "ERROR: Could not resolve home directory, aborting...");
		exit(-1);
	}

	if (argc > 1 && !strcmp(argv[1], "-v")) {
		print_version();
		return 0;
	}

	if (argc > 1 && !strcmp(argv[1], "-l")) {
		print_keys();
		return 0;
	}

	if (argc > 1 && !strcmp(argv[1], "-f"))
		foreground_flag++;

	sprintf(config_dir, "%s/.config/warpd", home);
	mkdir(config_dir, 0700);
	sprintf(config_path, "%s/config", config_dir);

	cfg = parse_cfg(config_path);

	lock();
	if (!foreground_flag)
		daemonize();

	printf("Starting warpd: "VERSION"\n");
	start_main_loop(main_loop);
}
