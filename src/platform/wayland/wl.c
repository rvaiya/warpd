#include "wayland.h"

struct screen screens[MAX_SCREENS];
int nr_screens;

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

		wl.pointer = wl_seat_get_pointer(seat);
		wl.keyboard = wl_seat_get_keyboard(seat);
		wl_seat_destroy(seat);
	}

	if (!strcmp(interface, "wl_output")) {
		/* TODO: test this in a multi screen environment. */
		struct screen *scr = &screens[nr_screens++];
		scr->wl_output = wl_registry_bind(registry, name, &wl_output_interface, 3);
	}

	if (!strcmp(interface, "zwlr_layer_shell_v1"))
		wl.layer_shell = wl_registry_bind(registry,
					 	  name, &zwlr_layer_shell_v1_interface, 4);
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

	if (!wl.pointer) {
		fprintf(stderr, "Could not get pointer object\n");
		exit(-1);
	}

	if (!wl.keyboard) {
		fprintf(stderr, "Could not get keyboard object\n");
		exit(-1);
	}

	if (!wl.layer_shell) {
		fprintf(stderr, "Could not get layer_shell object (unsupported protocol?)\n");
		exit(-1);
	}

	wl_pointer_set_cursor(wl.pointer, 0, NULL, 0, 0);

	init_input();
	init_screens();
}
