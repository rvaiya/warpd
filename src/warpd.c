/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include "warpd.h"

struct platform *platform = NULL;

static const char *config_path;

uint64_t get_time_us()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);

	return ts.tv_nsec / 1E3 + ts.tv_sec * 1E6;
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
		"  --screen                    Start warpd in screen selection mode and exit after the end of the session.\n"
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


static int drag_flag = 0;
static int oneshot_flag = 0;
static int click_flag = 0;
static int x_flag = -1;
static int y_flag = -1;
static int record_flag = 0;
static int mode = 0;

/* Platform entry points. */
int oneshot_main(struct platform *_platform)
{
	int ret = 0;
	screen_t scr;
	platform = _platform;

	parse_config(config_path);
	init_mouse();
	init_hints();

	platform->mouse_get_position(&scr, NULL, NULL);
	if (x_flag == -1 && y_flag == -1) {
		if (drag_flag)
			platform->mouse_down(config_get_int("drag_button"));

		ret = mode_loop(mode, oneshot_flag, record_flag);

		if (drag_flag)
			platform->mouse_up(config_get_int("drag_button"));

	} else {
		platform->mouse_move(scr, x_flag, y_flag);
	}

	if (click_flag)
		platform->mouse_click(click_flag);

	return ret;
}

int daemon_main(struct platform *_platform)
{
	platform = _platform;

	parse_config(config_path);
	init_mouse();
	init_hints();

	daemon_loop(config_path);

	return 0;
}

int print_keys_main(struct platform *platform)
{
	size_t i;
	for (i = 1; i < 256; i++) {
		const char *name = platform->input_lookup_name(i, 0);

		if (name && name[0])
			printf("%s\n", name);

		name = platform->input_lookup_name(i, 1);
		if (name && name[0])
			printf("%s\n", name);
	}

	return 0;
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
		{"screen", no_argument, NULL, 268},
		{0}
	};

	while ((c = getopt_long(argc, argv, "qrhfvlc:", opts, NULL)) != -1) {
		switch (c) {
			case 'v':
				print_version();
				return 0;
			case 'h':
				print_usage();
				return 0;
			case 'l':
				platform_run(print_keys_main);
				return 0;
			case 'c':
				config_path = optarg;
				break;
			case 'f':
				foreground = 1;
				break;
			case 'q':
				mode = MODE_HINTSPEC;
				oneshot_flag = 1;
				break;
			case 257:
				mode = MODE_HINT;
				break;
			case 258:
				mode = MODE_GRID;
				break;
			case 259:
				mode = MODE_NORMAL;
				break;
			case 261:
				mode = MODE_HINT2;
				break;
			case 262:
				mode = MODE_HISTORY;
				break;
			case 268:
				mode = MODE_SCREEN_SELECTION;
				break;
			case 263:
				if (!mode)
					mode = MODE_NORMAL;

				oneshot_flag = 1;
				break;
			case 264:
				click_flag = atoi(optarg);
				oneshot_flag = 1;
				break;
			case 265:
				sscanf(optarg, "%d %d", &x_flag, &y_flag);
				oneshot_flag = 1;
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

	if (mode || oneshot_flag) {
		platform_run(oneshot_main);
	} else {
		lock();

		if (!foreground)
			daemonize();

		setvbuf(stdout, NULL, _IOLBF, 0);
		printf("Starting warpd " VERSION "\n");

		platform_run(daemon_main);
	}
}
