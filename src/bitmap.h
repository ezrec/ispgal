/*
 * Copyright (C) 2010, Jason S. McMullan. All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
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

#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>
#include <stdio.h>

#define DECLARE_BITMAP(name, bits) \
	uint32_t __##name[((bits) + 31)/32], *name = &__##name[0]

static inline void bit_set(uint32_t *bitmap, int bit, int val)
{
	bitmap[bit / 32] &= ~(1 << (bit & 31));
	bitmap[bit / 32] |= ((val & 1) << (bit & 31));
}

static inline int bit_get(uint32_t *bitmap, int bit)
{
	return (bitmap[bit / 32] >> (bit & 31)) & 1;
}

static inline void bit_dump(uint32_t *bitmap, int bits)
{
	int i;

	for (i = 0; i < bits; i++)
		printf("%c", bit_get(bitmap, (bits - 1) - i) + '0');
}

static inline void bit_set_u32(uint32_t *bitmap, int bits, uint32_t val)
{
	int i;
	for (i = 0; i < bits; i++)
		bit_set(bitmap, i, (val >> i) & 1);
}

static inline void bit_ones(uint32_t *bitmap, int bits)
{
	int i;

	for (i = 0; i < bits; i++)
		bit_set(bitmap, i, 1);
}

static inline void bit_zeroes(uint32_t *bitmap, int bits)
{
	int i;

	for (i = 0; i < bits; i++)
		bit_set(bitmap, i, 0);
}

#endif /* BITMAP_H */
