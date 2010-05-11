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

#ifndef LSC_H
#define LSC_H

#include <stdint.h>
#include <sys/types.h>

struct lsc {
	const char *name;
	const char *help;
	int (*open)(struct lsc *lsc, const char *options);
	void (*close)(struct lsc *lsc);

	int (*Id)(struct lsc *lsc, unsigned int *id);
	int (*Run)(struct lsc *lsc, unsigned int cmd, unsigned int nsec);
	int (*Read)(struct lsc *lsc, unsigned int cmd, unsigned int bits, uint32_t *data_in);
	int (*Write)(struct lsc *lsc, unsigned int cmd, unsigned int bits, const uint32_t *data_out);

	size_t priv_size;
};

#define LSC_PRIV(x)	((void *)(&x[1]))

struct lsc *lsc_open(const char *type);

void lsc_close(struct lsc *lsc);

/* Get the ID */
static inline int lsc_Id(struct lsc *lsc, unsigned int *id)
{ return lsc->Id(lsc, id); }

/* Execute a command */
static inline int lsc_Run(struct lsc *lsc, unsigned int cmd, unsigned int nsec)
{ return lsc->Run(lsc, cmd, nsec); }
	
static inline int lsc_Read(struct lsc *lsc, unsigned int cmd, unsigned int bits, uint32_t *bitmap)
{ return lsc->Read(lsc, cmd, bits, bitmap); }

static inline int lsc_Write(struct lsc *lsc, unsigned int cmd, unsigned int bits, const uint32_t *bitmap)
{ return lsc->Write(lsc, cmd, bits, bitmap); }

#endif /* LSC_H */
