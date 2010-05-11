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

#include "lsc.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)	(sizeof(x)/sizeof((x)[0]))
#endif

extern const struct lsc lsc_jtagkey;

static const struct lsc *known_lscs[] = {
	&lsc_jtagkey,
};

struct lsc *lsc_open(const char *lsc_name)
{
	int i, err, len;
	struct lsc *lsc;
	const char *cp, *op;

	cp = strchr(lsc_name, ',');
	if (cp == NULL) {
		cp = lsc_name + strlen(lsc_name);
		op = cp;
	} else {
		op = cp+1;
	}
	len = cp - lsc_name;

	if (lsc_name == NULL) {
		fprintf(stderr, "Valid LSC tool types:\n");
		for (i = 0; i < ARRAY_SIZE(known_lscs); i++) {
			fprintf(stderr, "\t%s\n", known_lscs[i]->name);
		}
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < ARRAY_SIZE(known_lscs); i++) {
		if (strlen(known_lscs[i]->name) != len)
			continue;

		if (strncmp(known_lscs[i]->name, lsc_name, len) == 0)
			break;
	}

	if (i == ARRAY_SIZE(known_lscs))
		return NULL;

	lsc = malloc(sizeof(*lsc) + known_lscs[i]->priv_size);

	*lsc = *(known_lscs[i]);
	err = lsc->open(lsc, op);
	if (err < 0) {
		free(lsc);
		return NULL;
	}

	return lsc;
}

void lsc_close(struct lsc *lsc)
{
	lsc->close(lsc);
	free(lsc);
}
