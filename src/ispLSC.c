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
#include <time.h>

#include "bitmap.h"
#include "ispLSC.h"

static void nsleep(unsigned int nsec)
{
	struct timespec req = { .tv_sec = 0, .tv_nsec = nsec };
	nanosleep(&req, NULL);
}

int ispLSC_Read_ID(struct ispLSC *isp, uint8_t *id)
{
	int i;
	uint8_t in;

	isp->MODE(isp->priv, 1);
	isp->SDI(isp->priv, 0);

	nsleep(isp->T.su);

	isp->SCLK(isp->priv, 1);
	nsleep(isp->T.clkh);
	isp->SCLK(isp->priv, 0);

	in = 0;
	for (i = 0; i < 8; i++) {
		int val;

		isp->SDO(isp->priv, &val);

		in |= (val & 1) << i;

		isp->SCLK(isp->priv, 1);
		nsleep(isp->T.wh);
		isp->SCLK(isp->priv, 0);
		nsleep(isp->T.wl);
	}

	*id = in;
	return 0;
}

int ispLSC_Next_State(struct ispLSC *isp)
{
	isp->MODE(isp->priv, 1);
	isp->SDI(isp->priv, 1);

	nsleep(isp->T.su);

	isp->SCLK(isp->priv, 1);
	nsleep(isp->T.h);

	isp->MODE(isp->priv, 0);
	isp->SDI(isp->priv, 0);
	nsleep(isp->T.clkh);

	isp->SCLK(isp->priv, 0);

	return 0;
}

int ispLSC_Set_Command(struct ispLSC *isp, uint8_t cmd)
{
	int i;

	isp->MODE(isp->priv, 0);

	for (i = 0; i < 5; i++){
		isp->SDI(isp->priv, (cmd >> i) & 1);
		nsleep(isp->T.su);
		isp->SCLK(isp->priv, 1);
		nsleep(isp->T.clkh);
		isp->SCLK(isp->priv, 0);
	}
}

int ispLSC_Run_Command(struct ispLSC *isp)
{
	isp->MODE(isp->priv, 0);
	isp->SDI(isp->priv, 0);

	nsleep(isp->T.su);

	isp->SCLK(isp->priv, 1);
	nsleep(isp->T.wh);

	isp->SCLK(isp->priv, 0);
}

int ispLSC_Write_Data(struct ispLSC *isp, uint32_t *bitmap, int bits)
{
	int i, v;

	isp->MODE(isp->priv, 0);

	for (i = 0; i < bits; i++) {
		v = bit_get(bitmap, i);

		isp->SDI(isp->priv, v);

		nsleep(isp->T.su);
		isp->SCLK(isp->priv, 1);
		nsleep(isp->T.clkh);
		isp->SCLK(isp->priv, 0);
	}
}

int ispLSC_Read_Data(struct ispLSC *isp, uint32_t *bitmap, int bits)
{
	int i, v;

	isp->MODE(isp->priv, 0);

	nsleep(isp->T.su);

	for (i = 0; i < bits; i++) {
		isp->SCLK(isp->priv, 1);
		nsleep(isp->T.clkh);
		isp->SCLK(isp->priv, 0);

		isp->SDO(isp->priv, &v);

		bit_set(bitmap, i, v);
	}
}
