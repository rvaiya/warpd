/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include "warpd.h"

static int dragging = 0;

void toggle_drag()
{
	dragging = !dragging;

	if (dragging)
		platform_mouse_down(1);
	else
		platform_mouse_up(1);
}

static int oneshot_mode;

static void activation_loop(int mode)
{
	struct input_event *ev = NULL;

	dragging = 0;

	while (1) {
		switch (mode) {
		case MODE_HISTORY:
			if (history_hint_mode() < 0)
				goto exit;

			ev = NULL;
			mode = MODE_NORMAL;
			break;
		case MODE_NORMAL:
			ev = normal_mode(ev);

			if (config_input_match(ev, "history", 1))
				mode = MODE_HISTORY;
			else if (config_input_match(ev, "hint", 1))
				mode = MODE_HINT;
			else if (config_input_match(ev, "hint2", 1))
				mode = MODE_HINT2;
			else if (config_input_match(ev, "grid", 1))
				mode = MODE_GRID;
			else if (config_input_match(ev, "screen", 1))
				mode = MODE_SCREEN_SELECTION;
			else if (config_input_match(ev, "exit", 1) || !ev)
				goto exit;

			break;
		case MODE_HINT2:
		case MODE_HINT:
			if (full_hint_mode(mode == MODE_HINT2) < 0)
				goto exit;

			ev = NULL;
			mode = MODE_NORMAL;
			break;
		case MODE_GRID:
			ev = grid_mode();
			if (config_input_match(ev, "grid_exit", 1))
				ev = NULL;
			mode = MODE_NORMAL;
			break;
		case MODE_SCREEN_SELECTION:
			screen_selection_mode();
			mode = MODE_NORMAL;
			break;
		}
	}

exit:
	if (dragging)
		toggle_drag();
	return;
}

static void oneshot_loop()
{
	init_mouse();
	init_hints();

	activation_loop(oneshot_mode);
}

static void hintspec_loop()
{
	int x, y;

	init_mouse();
	init_hints();

	hintspec_mode();
	platform_mouse_get_position(NULL, &x, &y);
	printf("%d %d\n", x, y);
}

static void main_loop()
{
	init_mouse();
	init_hints();

	struct input_event activation_events[8] = {0};

	input_parse_string(&activation_events[0], config_get("activation_key"));
	input_parse_string(&activation_events[1], config_get("hint_activation_key"));
	input_parse_string(&activation_events[2], config_get("grid_activation_key"));
	input_parse_string(&activation_events[3], config_get("hint_oneshot_key"));
	input_parse_string(&activation_events[4], config_get("screen_activation_key"));
	input_parse_string(&activation_events[5], config_get("hint2_activation_key"));
	input_parse_string(&activation_events[6], config_get("hint2_oneshot_key"));
	input_parse_string(&activation_events[7], config_get("history_activation_key"));

	while (1) {
		int mode = 0;
		struct input_event *ev = platform_input_wait(activation_events,
						sizeof(activation_events)/sizeof(activation_events[0]));

		if (config_input_match(ev, "activation_key", 1))
			mode = MODE_NORMAL;
		else if (config_input_match(ev, "grid_activation_key", 1))
			mode = MODE_GRID;
		else if (config_input_match(ev, "hint_activation_key", 1))
			mode = MODE_HINT;
		else if (config_input_match(ev, "hint2_activation_key", 1))
			mode = MODE_HINT2;
		else if (config_input_match(ev, "screen_activation_key", 1))
			mode = MODE_SCREEN_SELECTION;
		else if (config_input_match(ev, "history_activation_key", 1))
			mode = MODE_HISTORY;
		else if (config_input_match(ev, "hint2_oneshot_key", 1)) {
			full_hint_mode(1);
			continue;
		} else if (config_input_match(ev, "hint_oneshot_key", 1)) {
			full_hint_mode(0);
			continue;
		} else if (config_input_match(ev, "history_oneshot_key", 1)) {
			history_hint_mode();
			continue;
		}

		activation_loop(mode);
	}
}

const char *get_config_path(const char *file)
{
	static char path[PATH_MAX];

	if (getenv("XDG_CONFIG_HOME")) {
		sprintf(path, "%s/warpd", getenv("XDG_CONFIG_HOME"));
		mkdir(path, 0700);
	} else {
		sprintf(path, "%s/.config", getenv("HOME"));
		mkdir(path, 0700);
		strcat(path, "/warpd");
		mkdir(path, 0700);
	}

	strcat(path, "/");
	strcat(path, file);

	return path;
}


static void lock()
{
	int fd;
	char path[64];

	sprintf(path, "/tmp/warpd_%d.lock", getuid());
	fd = open(path, O_RDONLY|O_CREAT, 0600);

	if (fd < 0) {
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
	if (fork())
		exit(0);
	if (fork())
		exit(0);

	int fd = open("/dev/null", O_WRONLY);
	if (fd < 0) {
		perror("open");
		exit(-1);
	}

	close(1);
	close(2);

	dup2(fd, 1);
	dup2(fd, 2);
}

static void print_keys_loop()
{
	size_t i;
	for (i = 0; i < 256; i++) {
		const char *name = platform_input_lookup_name(i);

		if (name && name[0])
			printf("%s\n", name);
	}
}

static void print_usage()
{
	const char *usage =
		"warpd: [options]\n\n"
		"  -f, --foreground            Run warpd in the foreground (useful for debugging).\n"
		"  -q, --query                 Consumes a list of hints from stdin and presents a one off hint selection.\n"
		"  -h, --help                  Print this help message.\n"
		"  -v, --version               Print the version and exit.\n"
		"  -c, --config <config file>  Use the supplied config file.\n"
		"  -l, --list-keys             Print all valid keys.\n"
		"  --list-options              Print all available config options.\n"

		"  --hint                      Start warpd in hint mode and exit after the end of the session.\n"
		"  --hint2                     Start warpd in two pass hint mode and exit after the end of the session.\n"
		"  --normal                    Start warpd in normal mode and exit after the end of the session.\n"
		"  --grid                      Start warpd in hint grid and exit after the end of the session.\n"
		;

	printf("%s", usage);
}

static void print_version()
{
	printf("warpd v" VERSION " (built from: " COMMIT ")\n");
}

int main(int argc, char *argv[])
{
	int c;
	int foreground = 0;
	const char *config_path = get_config_path("config");

	parse_config(config_path);

	struct option opts[] = {
		{"version", no_argument, NULL, 'v'},
		{"help", no_argument, NULL, 'h'},
		{"query", no_argument, NULL, 'q'},
		{"list-keys", no_argument, NULL, 'l'},
		{"foreground", no_argument, NULL, 'f'},
		{"config", required_argument, NULL, 'c'},

		{"hint", no_argument, NULL, 257},
		{"grid", no_argument, NULL, 258},
		{"normal", no_argument, NULL, 259},
		{"hint2", no_argument, NULL, 261},
		{"history", no_argument, NULL, 262},
		{"list-options", no_argument, NULL, 260},
		{0}
	};

	while ((c = getopt_long(argc, argv, "qrhfvlc:", opts, NULL)) != -1) {
		switch (c) {
			case 'v':
				print_version();
				return 0;
			case 'q':
				start_main_loop(hintspec_loop);
				return 0;
			case 'h':
				print_usage();
				return 0;
			case 'l':
				start_main_loop(print_keys_loop);
				return 0;
			case 'c':
				config_path = optarg;
				break;
			case 'f':
				foreground = 1;
				break;

			case 257:
				oneshot_mode = MODE_HINT;
				break;
			case 258:
				oneshot_mode = MODE_GRID;
				break;
			case 259:
				oneshot_mode = MODE_NORMAL;
				break;
			case 261:
				oneshot_mode = MODE_HINT2;
				break;
			case 262:
				oneshot_mode = MODE_HISTORY;
				break;
			case 260:
				config_print_options();
				return 0;
			case '?':
				return -1;
		}
	}

	lock();

	if (oneshot_mode) {
		start_main_loop(oneshot_loop);
		exit(0);
	}

	if (!foreground)
		daemonize();

	setvbuf(stdout, NULL, _IOLBF, 0);
	printf("Starting warpd v" VERSION " (" COMMIT ")\n");
	start_main_loop(main_loop);
}
