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
#include <malloc.h>
#include <string.h>

#include "chip.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)	(sizeof(x)/sizeof((x)[0]))
#endif

extern const struct chip chip_ispGAL22V10;

static struct {
	const char *name;
	const struct chip *chip;
} known_chips[] = {
	{ "GAL22V10", &chip_ispGAL22V10 },
	{ "ispGAL22V10", &chip_ispGAL22V10 },
};

struct chip *chip_detect(const char *name, const char *interface_type)
{
	int i, err;
	struct chip *chip;

	for (i = 0; i < ARRAY_SIZE(known_chips); i++) {
		if (strcasecmp(known_chips[i].name, name) == 0)
			break;
	}

	if (i == ARRAY_SIZE(known_chips))
		return NULL;

	chip = malloc(sizeof(*chip) + known_chips[i].chip->priv_size);

	*chip = *known_chips[i].chip;
	err = chip->init(chip, interface_type);
	if (err < 0) {
		free(chip);
		return NULL;
	}

	return chip;
}

