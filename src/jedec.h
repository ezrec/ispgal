/*
 * Copyright (C) 2010, Jason McMullan <jason.mcmullan@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef JEDEC_H
#define JEDEC_H

#include <stdint.h>

#include "bitmap.h"

struct jedec {
	const char *device;
	int bits;
	uint32_t *bitmap;
};

static inline void jedec_bit_set(struct jedec *jed, int fuse, int val)
{
	bit_set(jed->bitmap, fuse, val);
}

static inline int jedec_bit_get(struct jedec *jed, int fuse)
{
	return bit_get(jed->bitmap, fuse);
}

/* Return a new, erased JEDEC fusemap */
struct jedec *jedec_new(int bits);

void jedec_free(struct jedec *jed);

struct jedec *jedec_read(int fd);

#endif /* JEDEC_H */
