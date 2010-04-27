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
#include <unistd.h>
#include <stdint.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>

#include "jedec.h"

struct jedec *jedec_read(int fd)
{
	FILE *inf;
	int bit, bits, words;
	char buff[1024];
	struct jedec *jed;

	inf = fdopen(fd, "r");
	if (inf == NULL)
		return NULL;

	jed = NULL;

	while (fgets(buff, sizeof(buff)-1, inf) != NULL) {
printf("LINE: %s", buff);
		if (strncmp(buff, "*QF", 3) == 0) {
			bits = strtoul(&buff[3], NULL, 10);
			words = (bits + sizeof(uint32_t)*8 - 1)/(sizeof(uint32_t)*8);
			jed = malloc(sizeof(*jed) + words * sizeof(uint32_t));
			jed->bits = bits;
			jed->bitmap = (void *)&jed[1];
			memset(jed->bitmap, 0xff, words * sizeof(uint32_t));
			continue;
		}

		if (strncmp(buff, "*L", 2) == 0) {
			char *cp;

			bit = strtoul(&buff[2], &cp, 10);
			assert(*cp == ' ');
			cp++;
			while (*cp == '0' || *cp == '1') {
				jedec_bit_set(jed, bit++, *cp & 1);
				cp++;
			}
			continue;
		}
	}

	fclose(inf);

	return jed;
}

