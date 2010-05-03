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
#include <stdlib.h>

#include "jtag.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)	(sizeof(x)/sizeof((x)[0]))
#endif

extern const struct jtag jtag_jtagkey;
extern const struct jtag jtag_svf;

static const struct jtag *known_jtags[] = {
	&jtag_svf,
	&jtag_jtagkey,
};

struct jtag *jtag_open(const char *jtag_name)
{
	int i, err, len;
	struct jtag *jtag;
	const char *cp, *op;

	cp = strchr(jtag_name, ',');
	if (cp == NULL) {
		cp = jtag_name + strlen(jtag_name);
		op = cp;
	} else {
		op = cp+1;
	}
	len = cp - jtag_name;

	if (jtag_name == NULL) {
		fprintf(stderr, "Valid JTAG tool types:\n");
		for (i = 0; i < ARRAY_SIZE(known_jtags); i++) {
			fprintf(stderr, "\t%s\n", known_jtags[i]->name);
		}
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < ARRAY_SIZE(known_jtags); i++) {
		if (strlen(known_jtags[i]->name) != len)
			continue;

		if (strncmp(known_jtags[i]->name, jtag_name, len) == 0)
			break;
	}

	if (i == ARRAY_SIZE(known_jtags))
		return NULL;

	jtag = malloc(sizeof(*jtag) + known_jtags[i]->priv_size);

	*jtag = *(known_jtags[i]);
	err = jtag->open(jtag, op);
	if (err < 0) {
		free(jtag);
		return NULL;
	}

	return jtag;
}

void jtag_close(struct jtag *jtag)
{
	jtag->close(jtag);
	free(jtag);
}
