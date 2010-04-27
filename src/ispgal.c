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
#include <stdio.h>

#include "jedec.h"

static void dump_row_n(struct jedec *jed, int row)
{
	int i;

	printf("%2d: ", row);
	for (i = 0; i < 132; i++) {
		int v, fuse;

		fuse = (5764 + row) - 44*i;

		v = jedec_bit_get(jed, fuse);

		printf("%c", '0' + v);
	}
	printf("\n");
}

static void dump_row_ues(struct jedec *jed)
{
	int i;

	printf("%2d: ", 44);
	for (i = 0; i < 64; i++) {
		int v, fuse;

		fuse = 5891 - i;

		v = jedec_bit_get(jed, fuse);

		printf("%c", '0' + v);
	}
	printf("\n");
}

static void dump_row_asr(struct jedec *jed)
{
	int i;

	printf("ASR ");
	for (i = 0; i < 20; i++) {
		int v, fuse;

		fuse = 5826 - i;
		if (i & 1)
			fuse += 2;

		v = jedec_bit_get(jed, fuse);

		printf("%c", '0' + v);
	}
	printf("\n");
}

static void dump_jed(struct jedec *jed)
{
	int i;
	for (i = 0; i < 44; i++) {
		dump_row_n(jed, i);
	}
	dump_row_ues(jed);
	dump_row_asr(jed);
}

void program_jed(struct jedec *jed)
{
}

int main(int argc, char **argv)
{
	struct jedec *jed;

	jed = jedec_read(0);

	if (0) dump_jed(jed);

	program_jed(jed);

	return 0;
}
