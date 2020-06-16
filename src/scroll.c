#include <X11/extensions/Xinerama.h>
#include <X11/extensions/XTest.h>
#include <X11/Xlib.h>
#include <stdint.h>
#include "scroll.h"
#include "input.h"

static int _scroll(Display *dpy,
		   const int btn,
		   float *v,
		   float a,
		   uint16_t *key,
		   int timeout)
{
	//Non zero to provide the illusion of continuous scrolling
	const float stop_threshold = 8; 
	const float vf = 1000;

	int total_time = 0; //in ms
	float t = 0; //in ms
	int d = 0;
	float v0 = (*v <= stop_threshold) ? (stop_threshold + 1) : *v;

	while(1) {
		const int nd = (int)(v0*(t/1000) + .5 * a * (t/1000) * (t/1000));
		*v = (v0+(a * (t/1000)));

		if(a && *v >= vf) {
			t = 0;
			a = 0;
			d = 0;
			v0 = vf;
			continue;
		}

		int type = input_next_ev(1, key);
		if(type != EV_TIMEOUT && type != EV_KEYREPEAT)
			return type;

		if(*v <= stop_threshold) return -1;

		for (int i = 0; i < (nd-d); i++) {
			XTestFakeButtonEvent(dpy, btn, True, CurrentTime);
			XTestFakeButtonEvent(dpy, btn, False, CurrentTime);
			XFlush(dpy);
		}

		d = nd;
		t++;
		total_time++;

		if(timeout && total_time >= timeout)
			return EV_TIMEOUT;
	}
}

uint16_t scroll(Display *dpy,
		uint16_t start_key,
		int btn,
		float velocity,
		float acceleration,
		float fling_velocity,
		float fling_acceleration,
		float fling_deceleration,
		float fling_timeout)
{
	uint16_t key;

	float v = velocity;
	float a = acceleration;

	int exit = 0;
	int flung = 0;

	while(!exit) {
		int ev = _scroll(dpy, btn, &v, a, &key, 0);

		switch(ev) {
		case EV_KEYPRESS:
			if(key != start_key)
				return key;

			else if(flung && key == start_key)
				v += fling_velocity;
			break;
		case EV_KEYRELEASE:
			if(key == start_key) {
				if(flung) {
					a = -fling_deceleration;
				} else {
					int ev;
					switch(ev = input_next_ev(fling_timeout, &key)) {
					case EV_KEYPRESS:
						if(key == start_key) {
							v += fling_velocity;
							a = a;
							flung = 1;
						} else
							return key;
						break;
					default:
						exit = 1;
					}
				}
			}
			break;
		case -1: //Stopped
			exit = 1;
			break;
		}
	}

	return 0;
}
