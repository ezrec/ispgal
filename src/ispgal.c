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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "jedec.h"
#include "chip.h"

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

int main(int argc, char **argv)
{
	struct jedec *jed_in, *jed_out;
	struct chip *chip;
	int err;

//	chip = chip_detect("ispGAL22V10", "jtagkey");
	chip = chip_detect("ispGAL22LV10", "svf");
	assert(chip != NULL);

	err = chip_diagnose(chip);
	if (err < 0)
		return EXIT_FAILURE;

	chip_erase(chip);

	jed_in = jedec_read(0);
	chip_program(chip, jed_in);

	jed_out = jedec_new(5892);
	chip_verify(chip, jed_in);

//	dump_jed(jed_out);

	return 0;
}
