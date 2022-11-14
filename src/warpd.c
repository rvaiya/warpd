/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include "warpd.h"

static int dragging = 0;

void toggle_drag()
{
	int btn = config_get_int("drag_button");
	dragging = !dragging;

	if (dragging)
		platform.mouse_down(btn);
	else
		platform.mouse_up(btn);
}

static int mode_flag = 0;
static int click_arg = 0;
static int movearg_x = -1;
static int movearg_y = -1;
static int record_flag = 0;
static int drag_flag = 0;

struct platform platform = {0};

static const char *config_path;

static int oneshot_mode = 0;

static int activation_loop(int mode)
{
	static int init = 0;
	int oneshot_normal = 0;
	int rc = 0;

	if (!init) {
		init_mouse();
		init_hints();

		if (drag_flag)
			toggle_drag();

		init++;
	}

	if (oneshot_mode && mode == MODE_NORMAL) {
		/*
		 * If oneshot is used in conjunction with normal
		 * we allow other intermediate modes before the
		 * final selection.
		 */
		oneshot_normal = 1;
	}

	struct input_event *ev = NULL;

	dragging = 0;

	config_input_whitelist(NULL, 0);

	while (1) {
		switch (mode) {
		case MODE_HISTORY:
			if (history_hint_mode() < 0)
				goto exit;

			ev = NULL;
			mode = MODE_NORMAL;
			break;
		case MODE_HINTSPEC:
			hintspec_mode();
			break;
		case MODE_NORMAL:
			ev = normal_mode(ev, oneshot_mode);

			if (config_input_match(ev, "history"))
				mode = MODE_HISTORY;
			else if (config_input_match(ev, "hint"))
				mode = MODE_HINT;
			else if (config_input_match(ev, "hint2"))
				mode = MODE_HINT2;
			else if (config_input_match(ev, "grid"))
				mode = MODE_GRID;
			else if (config_input_match(ev, "screen"))
				mode = MODE_SCREEN_SELECTION;
			else if ((rc = config_input_match(ev, "oneshot_buttons")) || !ev) {
				goto exit;
			}
			else if (config_input_match(ev, "exit") || !ev) {
				rc = 0;
				goto exit;
			}

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
			if (config_input_match(ev, "grid_exit"))
				ev = NULL;
			mode = MODE_NORMAL;
			break;
		case MODE_SCREEN_SELECTION:
			screen_selection_mode();
			mode = MODE_NORMAL;
			ev = NULL;
			break;
		}

		if (oneshot_mode &&
			(!oneshot_normal || config_input_match(ev, "buttons"))) {
			int rc = 0;
			int x, y;
			screen_t scr;

			platform.mouse_get_position(&scr, NULL, NULL);

			if (movearg_x != -1 && movearg_y != -1)
				platform.mouse_move(scr, movearg_x, movearg_y);

			if (click_arg) {
				platform.mouse_click(click_arg);
				rc = click_arg;
			}

			platform.mouse_get_position(NULL, &x, &y);

			if (record_flag)
				histfile_add(x, y);

			if (mode == MODE_HINTSPEC)
				printf("%d %d %s\n", x, y, last_selected_hint);
			else
				printf("%d %d\n", x, y);
			exit(rc);
		}
	}

exit:
	if (dragging)
		toggle_drag();

	return rc;
}

static void mode_loop()
{
	parse_config(config_path);

	exit(activation_loop(mode_flag));
}

static void daemon_loop()
{
	size_t i;
	parse_config(config_path);

	init_mouse();
	init_hints();

	const char *activation_keys[] = {
		"activation_key",
		"hint_activation_key",
		"grid_activation_key",
		"hint_oneshot_key",
		"screen_activation_key",
		"hint2_activation_key",
		"hint2_oneshot_key",
		"history_activation_key",
	};

	struct input_event activation_events[sizeof activation_keys / sizeof activation_keys[0]];

	for (i = 0; i < sizeof activation_keys / sizeof activation_keys[0]; i++)
		input_parse_string(&activation_events[i], config_get(activation_keys[i]));

	while (1) {
		int mode = 0;
		struct input_event *ev = platform.input_wait(activation_events,
							     sizeof(activation_events) /
							     sizeof(activation_events[0]));

		config_input_whitelist(activation_keys, sizeof activation_keys / sizeof activation_keys[0]);

		if (config_input_match(ev, "activation_key"))
			mode = MODE_NORMAL;
		else if (config_input_match(ev, "grid_activation_key"))
			mode = MODE_GRID;
		else if (config_input_match(ev, "hint_activation_key"))
			mode = MODE_HINT;
		else if (config_input_match(ev, "hint2_activation_key"))
			mode = MODE_HINT2;
		else if (config_input_match(ev, "screen_activation_key"))
			mode = MODE_SCREEN_SELECTION;
		else if (config_input_match(ev, "history_activation_key"))
			mode = MODE_HISTORY;
		else if (config_input_match(ev, "hint2_oneshot_key")) {
			full_hint_mode(1);
			continue;
		} else if (config_input_match(ev, "hint_oneshot_key")) {
			full_hint_mode(0);
			continue;
		} else if (config_input_match(ev, "history_oneshot_key")) {
			history_hint_mode();
			continue;
		}

		activation_loop(mode);
	}
}

const char *get_data_path(const char *file)
{
	static char path[PATH_MAX];

	if (getenv("XDG_DATA_DIR")) {
		sprintf(path, "%s/warpd", getenv("XDG_DATA_DIR"));
		mkdir(path, 0700);
	} else {
		sprintf(path, "%s/.local", getenv("HOME"));
		mkdir(path, 0700);
		strcat(path, "/share");
		mkdir(path, 0700);
		strcat(path, "/warpd");
		mkdir(path, 0700);
	}

	strcat(path, "/");
	strcat(path, file);

	return path;
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
		exit(-1);
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
	for (i = 1; i < 256; i++) {
		const char *name = platform.input_lookup_name(i, 0);

		if (name && name[0])
			printf("%s\n", name);

		name = platform.input_lookup_name(i, 1);
		if (name && name[0])
			printf("%s\n", name);
	}
}

static void print_usage()
{
	const char *usage =
		"warpd: [options]\n\n"
		"  -f, --foreground            Run warpd in the foreground (useful for debugging).\n"
		"  -h, --help                  Print this help message.\n"
		"  -v, --version               Print the version and exit.\n"
		"  -c, --config <config file>  Use the supplied config file.\n"
		"  -l, --list-keys             Print all valid keys.\n"
		"  --list-options              Print all available config options.\n"

		"  --hint                      Start warpd in hint mode and exit after the end of the session.\n"
		"  --hint2                     Start warpd in two pass hint mode and exit after the end of the session.\n"
		"  --normal                    Start warpd in normal mode and exit after the end of the session.\n"
		"  --grid                      Start warpd in hint grid and exit after the end of the session.\n"
		"  --oneshot                   When paired with one of the mode flags, exit warpd as soon as the mode is complete (i.e don't drop into normal mode). Principally useful for scripting.\n"
		"  --move '<x> <y>'            Move the pointer to the specified coordinates.\n"
		"  --click <button>            Send a mouse click corresponding to the supplied button and exit. May be paired with --move.\n"
		"  -q, --query                 Consumes a list of hints from stdin and presents a one off hint selection.\n"
		"  --record                    When used with --click, records the event in warpd's hint history.\n\n"
		;

	printf("%s", usage);
}

static void print_version()
{
	printf("warpd " VERSION"\n");
}

int main(int argc, char *argv[])
{
	int c;
	int foreground = 0;
	config_path = get_config_path("config");

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
		{"oneshot", no_argument, NULL, 263},
		{"click", required_argument, NULL, 264},
		{"move", required_argument, NULL, 265},
		{"record", no_argument, NULL, 266},
		{"drag", no_argument, NULL, 267},
		{0}
	};

	platform_init();

	while ((c = getopt_long(argc, argv, "qrhfvlc:", opts, NULL)) != -1) {
		switch (c) {
			case 'v':
				print_version();
				return 0;
			case 'h':
				print_usage();
				return 0;
			case 'l':
				platform.run(print_keys_loop);
				return 0;
			case 'c':
				config_path = optarg;
				break;
			case 'f':
				foreground = 1;
				break;
			case 'q':
				mode_flag = MODE_HINTSPEC;
				oneshot_mode = 1;
				break;
			case 257:
				mode_flag = MODE_HINT;
				break;
			case 258:
				mode_flag = MODE_GRID;
				break;
			case 259:
				mode_flag = MODE_NORMAL;
				break;
			case 261:
				mode_flag = MODE_HINT2;
				break;
			case 262:
				mode_flag = MODE_HISTORY;
				break;
			case 263:
				if (!mode_flag)
					mode_flag = MODE_NORMAL;

				oneshot_mode = 1;
				break;
			case 264:
				click_arg = atoi(optarg);
				oneshot_mode = 1;
				break;
			case 265:
				sscanf(optarg, "%d %d", &movearg_x, &movearg_y);
				oneshot_mode = 1;
				break;
			case 266:
				record_flag = 1;
				break;
			case 267:
				drag_flag = 1;
				break;
			case 260:
				config_print_options();
				return 0;
			case '?':
				return -1;
		}
	}

	if (mode_flag || oneshot_mode) {
		platform.run(mode_loop);
	} else {
		lock();

		if (!foreground)
			daemonize();

		setvbuf(stdout, NULL, _IOLBF, 0);
		printf("Starting warpd " VERSION "\n");
		platform.run(daemon_loop);
	}
}
