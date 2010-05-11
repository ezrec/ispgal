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

#ifndef JTAG_H
#define JTAG_H

#include <stdint.h>

struct jtag {
	const char *name;
	const char *help;
	int (*open)(struct jtag *jtag, const char *options);
	void (*close)(struct jtag *jtag);
	int (*nsleep)(struct jtag *jtag, unsigned int nsec);
	int (*IR)(struct jtag *jtag, unsigned int bits, uint32_t *in, uint32_t *out);
	int (*DR)(struct jtag *jtag, unsigned int bits, uint32_t *in, uint32_t *out);

	size_t priv_size;
};

#define JTAG_PRIV(x)	((void *)(&x[1]))

struct jtag *jtag_open(const char *type);

void jtag_close(struct jtag *jtag);

/* Send an IR command */
static inline int jtag_IR(struct jtag *jtag, unsigned int bits, uint32_t *in, uint32_t *out)
{ return jtag->IR(jtag, bits, in, out); }
	
/* Send a DR command, and (optionally) verify its response 
 *
 * *out is the expected output, and the return value will
 * be -ESRCH if it does not match.
 *
 * *out will be overridden by the value received.
 */
static inline int jtag_DR(struct jtag *jtag, unsigned int bits, uint32_t *in, uint32_t *out)
{ return jtag->DR(jtag, bits, in, out); }

/* Idle for a bit */
static inline int jtag_nsleep(struct jtag *jtag, unsigned int nsec)
{ return jtag->nsleep(jtag, nsec); }

#endif /* JTAG_H */
