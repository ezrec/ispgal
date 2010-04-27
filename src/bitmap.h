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

static inline void bit_set(uint32_t *bitmap, int bit, int val)
{
	bitmap[bit / 32] &= ~(1 << (bit & 31));
	bitmap[bit / 32] |= ((val & 1) << (bit & 31));
}

static inline int bit_get(uint32_t *bitmap, int bit)
{
	return (bitmap[bit / 32] >> (bit & 31)) & 1;
}


#endif /* BITMAP_H */
