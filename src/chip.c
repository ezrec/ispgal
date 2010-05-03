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
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#include "chip.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)	(sizeof(x)/sizeof((x)[0]))
#endif

extern const struct chip chip_ispGAL22V10;
extern const struct chip chip_ispGAL22LV10;

static const struct chip *known_chips[] = {
	&chip_ispGAL22V10,
	&chip_ispGAL22LV10,
};

struct chip *chip_open(const char *chip_name, const char *tool_name)
{
	int i, len, err;
	struct chip *chip;
	const char *cp, *op;

	cp = strchr(chip_name, ',');
	if (cp == NULL) {
		cp = chip_name + strlen(chip_name);
		op = cp;
	} else {
		op = cp+1;
	}
	len = cp - chip_name;

	if (chip_name == NULL) {
		fprintf(stderr, "Valid chip types:\n");
		for (i = 0; i < ARRAY_SIZE(known_chips); i++) {
			fprintf(stderr, "\t%s\n", known_chips[i]->name);
		}
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < ARRAY_SIZE(known_chips); i++) {
		if (strlen(known_chips[i]->name) != len)
			continue;

		if (strncmp(known_chips[i]->name, chip_name, len) == 0)
			break;
	}

	if (i == ARRAY_SIZE(known_chips))
		return NULL;

	chip = malloc(sizeof(*chip) + known_chips[i]->priv_size);

	*chip = *known_chips[i];
	err = chip->open(chip, op, tool_name);
	if (err < 0) {
		free(chip);
		return NULL;
	}

	return chip;
}

void chip_close(struct chip *chip)
{
	chip->close(chip);
	free(chip);
}
