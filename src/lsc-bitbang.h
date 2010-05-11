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

#ifndef LSC_BITBANG_H
#define LSC_BITBANG_H

#include <stdint.h>

#include "lsc.h"

/* Use this struct as your private data for an LSC interface
 */
struct lsc_bitbang {
	int (*MODE)(void *priv, int v);	
	int (*SDI)(void *priv, int v);
	int (*SCLK)(void *priv, int v);
	int (*SDO)(void *priv, int *v);
	void (*nsleep)(void *priv, unsigned int nsec);
	void *priv;

	struct {
		unsigned int su;	/* nsec */
		unsigned int h;		/* nsec */
		unsigned int co;	/* nsec */
		unsigned int clkh;	/* nsec */
		unsigned int clkl;	/* nsec */
	} T;

	enum {
		LSC_STATE_IDLE,
		LSC_STATE_SHIFT,
		LSC_STATE_EXECUTE,
	} state;
};

/* Use these as your functions in the 'struct lsc'
 */
int lsc_bitbang_Id(struct lsc *lsc, unsigned int *id);
int lsc_bitbang_Run(struct lsc *lsc, unsigned int cmd, unsigned int nsec);
int lsc_bitbang_Read(struct lsc *lsc, unsigned int cmd,
                     unsigned int bits, uint32_t *bitmap);
int lsc_bitbang_Write(struct lsc *lsc, unsigned int cmd,
                      unsigned int bits, const uint32_t *bitmap);
int lsc_bitbang_Diagnose(struct lsc *lsc);

#endif /* LSC_BITBANG_H */
