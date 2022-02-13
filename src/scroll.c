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

#ifdef __APPLE__
const int factor = 1;
#else
const int factor = 50;
#endif

int fling_velocity = 2000/factor;

static long last_tick = 0;

/* terminal velocity */
const float vt = 9000/factor;
const float v0 = 300/factor;

const float da0 = -3400/factor; /* deceleration */

const float a0 = 1600/factor;

/* in scroll units per second. */
static float v = 0;
static float a = 0;
static float d = 0; /* total distance */

static int direction = 0;

static long traveled = 0; /* scroll units emitted. */


static long get_time_ms()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_nsec/1E6 + ts.tv_sec*1E3;
}

void scroll_tick()
{
	int i;
	/* Non zero to provide the illusion of continuous scrolling */

	const float t = (float)(get_time_ms()-last_tick); //time elapsed since last tick in ms
	last_tick = get_time_ms();

	/* distance traveled since the last tick */
	d += v*(t/1000) + .5 * a * (t/1000) * (t/1000);
	v += a * (t/1000);

	if (v < 0) {
		v = 0;
		d = 0;
		traveled = 0;
	}

	if (v >= vt) {
		v = vt;
		a = 0;
	}

	for (i = 0; i < (long)d-traveled; i++)
		scroll(direction);

	traveled = (long) d;
}

void scroll_stop()
{
	v = 0;
	a = 0;
	traveled = 0;
	d = 0;
}

void scroll_decelerate()
{
	a = da0;
}

void scroll_accelerate(int _direction)
{
	direction = _direction;
	a = a0;

	if (v == 0) {
		d = 0;
		traveled = 0;
		v = v0;
	}
}

void scroll_impart_impulse()
{
	v += fling_velocity;
}
