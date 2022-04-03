/*
 * warpd - A keyboard-driven modal pointer.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include "warpd.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/stat.h>

struct cfg *cfg;
static int  dragging = 0;
char	    config_dir[512];

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

static void normalize_dimensions()
{
	int sw, sh;

	screen_get_dimensions(&sw, &sh);

	cfg->speed = (cfg->speed * sh) / 1080;
	cfg->hint_size = (cfg->hint_size * sh) / 1080;
	cfg->cursor_size = (cfg->cursor_size * sh) / 1080;
	cfg->grid_size = (cfg->grid_size * sh) / 1080;
	cfg->grid_border_size = (cfg->grid_border_size * sh) / 1080;
}

static void main_loop()
{
	normalize_dimensions();

	init_mouse();
	init_grid_mode();
	init_normal_mode();
	init_hint_mode();

	struct input_event activation_events[4] = {0};

	input_parse_string(&activation_events[0], cfg->activation_key);
	input_parse_string(&activation_events[1], cfg->hint_activation_key);
	input_parse_string(&activation_events[2], cfg->grid_activation_key);
	input_parse_string(&activation_events[3], cfg->hint_oneshot_key);

	while (1) {
		int		    mode = 0;
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
	int  fd;
	sprintf(path, "%s/lock", config_dir);

	if ((fd = open(path, O_CREAT | O_RDWR, 0600)) == -1) {
		perror("flock open");
		exit(1);
	}

	if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
		fprintf(
		    stderr,
		    "ERROR: Another instance of warpd is already running.\n");
		exit(-1);
	}
}

static void daemonize()
{
	char path[1024];

	if (fork())
		exit(0);
	if (fork())
		exit(0);

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
	size_t i;
	for (i = 0; i < 256; i++) {
		const char *name = input_lookup_name(i);

		if (name)
			printf("%s\n", name);
	}
}

static void print_version()
{
	printf("warpd v" VERSION " (built from: " COMMIT ")\n");
}

int main(int argc, char *argv[])
{
	int	    foreground_flag = 0;
	char	    config_path[1024];
	const char *home = getenv("HOME");

	if (!home) {
		fprintf(stderr,
			"ERROR: Could not resolve home directory, aborting...");
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

	setvbuf(stdout, NULL, _IOLBF, 0);
	printf("Starting warpd: " VERSION "\n");

	start_main_loop(main_loop);
}
