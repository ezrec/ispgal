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

#ifndef ISPLSC_H
#define ISPLSC_H

#include <stdint.h>

struct ispLSC {
	int (*MODE)(void *priv, int v);	
	int (*SDI)(void *priv, int v);
	int (*SCLK)(void *priv, int v);
	int (*SDO)(void *priv, int *v);

	struct {
		unsigned int su;	/* In ns */
		unsigned int clkh;	/* In ns */
		unsigned int wh;	/* In ns */
		unsigned int wl;	/* In ns */
		unsigned int h;		/* In ns */
	} T;

	void *priv;
};


int ispLSC_Read_ID(struct ispLSC *isp, uint8_t *id);

int ispLSC_Next_State(struct ispLSC *isp);

int ispLSC_Set_Command(struct ispLSC *isp, uint8_t cmd);

int ispLSC_Run_Command(struct ispLSC *isp);

int ispLSC_Write_Data(struct ispLSC *isp, uint32_t *bitmap, int bits);

int ispLSC_Read_Data(struct ispLSC *isp, uint32_t *bitmap, int bits);


#endif /* ISPLSC_H */
