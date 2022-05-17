#include "wayland.h"

struct screen screens[MAX_SCREENS];
size_t nr_screens;

struct wl wl;

static void handle_global(void *data,
			  struct wl_registry *registry, uint32_t name,
			  const char *interface, uint32_t version)
{
	if (!strcmp(interface, "zwlr_virtual_pointer_manager_v1")) {
		struct zwlr_virtual_pointer_manager_v1 *mgr;

		mgr = wl_registry_bind(registry, name, &zwlr_virtual_pointer_manager_v1_interface, 1);
		wl.ptr = zwlr_virtual_pointer_manager_v1_create_virtual_pointer(mgr, NULL);
	}

	if (!strcmp(interface, "wl_shm"))
		wl.shm = wl_registry_bind(registry,
					  name, &wl_shm_interface, 1);

	if (!strcmp(interface, "wl_compositor"))
		wl.compositor = wl_registry_bind(registry,
						 name, &wl_compositor_interface, 4);

	if (!strcmp(interface, "wl_seat")) {
		struct wl_seat *seat =
			wl_registry_bind(registry,
					 name, &wl_seat_interface, 7);

		add_seat(seat);
	}

	if (!strcmp(interface, "wl_output")) {
		struct wl_output *output = wl_registry_bind(registry, name, &wl_output_interface, 3);
		add_screen(output);
	}

	if (!strcmp(interface, "zxdg_output_manager_v1")) {
		wl.xdg_output_manager = wl_registry_bind(registry, name, &zxdg_output_manager_v1_interface, 3);
	}

	if (!strcmp(interface, "zwlr_layer_shell_v1"))
		wl.layer_shell = wl_registry_bind(registry,
						  name, &zwlr_layer_shell_v1_interface, 2);
}

static struct wl_registry_listener registry_listener = {
	.global = handle_global,
};

void init_wl()
{
	wl.dpy = wl_display_connect(NULL);

	if (!wl.dpy) {
		fprintf(stderr, "Failed to connect to wayland server\n");
		exit(-1);
	}

	wl_registry_add_listener(wl_display_get_registry(wl.dpy), &registry_listener, NULL);

	wl_display_dispatch(wl.dpy);

	if (!wl.ptr) {
		fprintf(stderr, "Could not create virtual pointer (virtual pointer unsupported?)\n");
		exit(-1);
	}

	if (!wl.compositor) {
		fprintf(stderr, "Could not get compositor object\n");
		exit(-1);
	}

	if (!wl.shm) {
		fprintf(stderr, "Could not get shm object\n");
		exit(-1);
	}

	if (!wl.xdg_output_manager) {
		fprintf(stderr, "Could not get xdg_output_manager object (unsupported protocol?)\n");
		exit(-1);
	}

	if (!wl.layer_shell) {
		fprintf(stderr, "Could not get layer_shell object (unsupported protocol?)\n");
		exit(-1);
	}

	init_screen();
}
