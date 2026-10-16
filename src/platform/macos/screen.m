/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include "macos.h"

struct screen screens[MAX_SCREENS];
size_t nr_screens;

static void draw_hook(void *arg, NSView *view)
{
	struct box *b = arg;
	macos_draw_box(b->scr, b->color, b->x, b->y, b->w, b->h, 0);
}

void osx_screen_draw_box(struct screen *scr, int x, int y, int w, int h, const char *color)
{
	assert(scr->nr_boxes < MAX_BOXES);
	struct box *b = &scr->boxes[scr->nr_boxes++];

	b->x = x;
	b->y = y;
	b->w = w;
	b->h = h;
	b->scr = scr;
	b->color = nscolor_from_hex(color);

	window_register_draw_hook(scr->overlay, draw_hook, b);
}

void osx_screen_list(struct screen *rscreens[MAX_SCREENS], size_t *n)
{
	size_t i;

	for (i = 0; i < nr_screens; i++)
		rscreens[i] = &screens[i];

	*n = nr_screens;
}

void osx_screen_clear(struct screen *scr)
{
	scr->nr_boxes = 0;
	scr->overlay->nr_hooks = 0;
}

void osx_screen_get_dimensions(struct screen *scr, int *w, int *h)
{
	*w = scr->w;
	*h = scr->h;
}

/* Only accessed on the main thread. */
static int screens_dirty = 0;

/*
 * (Re)build the screen list from NSScreen.screens. Must run on the main
 * thread while no mode is active.
 *
 * Entries are updated in place and overlay windows are reused rather than
 * freed, so screen_t pointers held by the core (e.g grid.c, mouse.c) always
 * point to valid memory. The core re-fetches them at the start of every mode.
 */
static void update_screens()
{
	size_t i;
	size_t n = 0;

	for (NSScreen *screen in NSScreen.screens) {
		struct screen *scr;
		NSRect frame = screen.frame;

		if (n == MAX_SCREENS) {
			fprintf(stderr, "WARNING: more than %d screens, ignoring the rest\n", MAX_SCREENS);
			break;
		}

		scr = &screens[n++];

		scr->x = frame.origin.x;
		scr->y = frame.origin.y;
		scr->w = frame.size.width;
		scr->h = frame.size.height;

		scr->nr_hints = 0;
		scr->nr_boxes = 0;

		if (scr->overlay)
			[scr->overlay->win setFrame:frame display:NO];
		else
			scr->overlay = create_overlay_window(scr->x, scr->y, scr->w, scr->h);

		scr->overlay->nr_hooks = 0;
		window_hide(scr->overlay);
	}

	/* Hide overlays belonging to screens which have gone away. */
	for (i = n; i < nr_screens; i++) {
		screens[i].overlay->nr_hooks = 0;
		window_hide(screens[i].overlay);
	}

	nr_screens = n;
	screens_dirty = 0;
}

/*
 * Apply pending display configuration changes (hotplug, rearrangement,
 * resolution changes). Called from the warpd thread while idle.
 */
void macos_sync_screens()
{
	dispatch_sync(dispatch_get_main_queue(), ^{
		if (screens_dirty)
			update_screens();
	});
}

void macos_init_screen()
{
	update_screens();

	[[NSNotificationCenter defaultCenter]
	    addObserverForName:NSApplicationDidChangeScreenParametersNotification
			object:nil
			 queue:[NSOperationQueue mainQueue]
		    usingBlock:^(NSNotification *note) {
			    screens_dirty = 1;
			    osx_input_interrupt_screen_change();
		    }];
}
