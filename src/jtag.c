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

#include "jtag.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)	(sizeof(x)/sizeof((x)[0]))
#endif

extern const struct jtag jtag_jtagkey;
extern const struct jtag jtag_svf;

static struct {
	const char *name;
	const struct jtag *jtag;
} known_jtags[] = {
	{ "svf", &jtag_svf },
	{ "jtagkey", &jtag_jtagkey },
};

struct jtag *jtag_detect(const char *name)
{
	int i, err;
	struct jtag *jtag;

	for (i = 0; i < ARRAY_SIZE(known_jtags); i++) {
		if (strcasecmp(known_jtags[i].name, name) == 0)
			break;
	}

	if (i == ARRAY_SIZE(known_jtags))
		return NULL;

	jtag = malloc(sizeof(*jtag) + known_jtags[i].jtag->priv_size);

	*jtag = *known_jtags[i].jtag;
	err = jtag->init(jtag);
	if (err < 0) {
		free(jtag);
		return NULL;
	}

	return jtag;
}

