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

#ifndef __H_INPUT_
#define __H_INPUT_

#define TIMEOUT_KEYSEQ (1<<7)

#define EV_XEV 1
#define EV_DEVICE_CHANGE 2
#define EV_KEYPRESS 3
#define EV_KEYRELEASE 4
#define EV_KEYREPEAT 5
#define EV_MOD 6
#define EV_TIMEOUT 7

void init_input(Display *_dpy);

uint16_t input_wait_for_key(uint16_t *keys, size_t n);

const char* input_keyseq_to_string(uint16_t seq);
uint16_t input_parse_keyseq(const char* key);

void input_click(int btn);
void input_grab_keyboard();
void input_ungrab_keyboard(int wait_for_keyboard);

uint16_t input_next_key(int timeout, int include_repeats);
int input_next_ev(int timeout, uint16_t *keyseq);
void input_get_cursor_position(int *x, int *y);
#endif
