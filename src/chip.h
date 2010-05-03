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

#ifndef CHIP_H
#define CHIP_H

#include "jedec.h"

struct chip {
	const char *name;
	const char *help;
	int (*open)(struct chip *chip, const char *options, const char *tool);
	void (*close)(struct chip *chip);
	int (*erase)(struct chip *chip);
	int (*program)(struct chip *chip, struct jedec *jed);
	int (*verify)(struct chip *chip, struct jedec *jed);
	int (*diagnose)(struct chip *chip);

	size_t priv_size;
};

#define CHIP_PRIV(chip)	((void *)&chip[1])

struct chip *chip_open(const char *name, const char *interface_type);

void chip_close(struct chip *chip);

static inline int chip_erase(struct chip *chip)
{ return chip->erase(chip); }

static inline int chip_program(struct chip *chip, struct jedec *jed)
{ return chip->program(chip, jed); }

static inline int chip_verify(struct chip *chip, struct jedec *jed)
{ return chip->verify(chip, jed); }

static inline int chip_diagnose(struct chip *chip)
{ return chip->diagnose(chip); }

#endif /* CHIP_H */
