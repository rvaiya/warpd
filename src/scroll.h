#ifndef _SCROLL_H_
#define _SCROLL_H_

uint16_t scroll(Display *dpy,
		uint16_t start_key,
		int btn,
		float velocity,
		float acceleration,
		float fling_velocity,
		float fling_acceleration,
		float fling_deceleration,
		float fling_timeout);

#endif
