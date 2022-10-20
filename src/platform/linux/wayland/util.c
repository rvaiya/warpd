/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */
#include "wayland.h"

int wl_hex_to_rgba(const char *str, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a)
{
#define X2B(c) ((c >= '0' && c <= '9') ? (c & 0xF) : (((c | 0x20) - 'a') + 10))

	if (str == NULL)
		return 0;
	str = (*str == '#') ? str + 1 : str;

	ssize_t len = strlen(str);

	if (len != 6 && len != 8)
		return -1;

	*r = X2B(str[0]);
	*r <<= 4;
	*r |= X2B(str[1]);

	*g = X2B(str[2]);
	*g <<= 4;
	*g |= X2B(str[3]);

	*b = X2B(str[4]);
	*b <<= 4;
	*b |= X2B(str[5]);

	*a = 255;
	if (len == 8) {
		*a = X2B(str[6]);
		*a <<= 4;
		*a |= X2B(str[7]);
	}

	return 0;
}
