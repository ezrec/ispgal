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

#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "bitmap.h"
#include "ispLSC.h"
#include "ispGAL.h"

void nsleep(unsigned int nsec)
{
	struct timespec req = { .tv_sec = 0, .tv_nsec = nsec };
	nanosleep(&req, NULL);
}

int ispLSC_Idle(struct ispLSC *isp)
{
	isp->MODE(isp->priv, 1);
	isp->SDI(isp->priv, 0);

	nsleep(isp->T.su);

	isp->SCLK(isp->priv, 1);
	nsleep(isp->T.clkh);
	isp->SCLK(isp->priv, 0);
}

int ispLSC_Read_ID(struct ispLSC *isp, uint8_t *id)
{
	int i;
	uint8_t in;

	/* Clock ID into the shift register
	 */
	isp->MODE(isp->priv, 1);
	isp->SDI(isp->priv, 0);
	nsleep(isp->T.su);

	isp->state = ISP_STATE_IDLE;

	for (i = 0; i < 8; i++) {
		isp->SCLK(isp->priv, 1);
		nsleep(isp->T.clkh);
		isp->SCLK(isp->priv, 0);
		nsleep(isp->T.clkl);
	}

	/* Clock shift register (with ID) 
	 * out to SDO
	 */
	isp->MODE(isp->priv, 0);
	isp->SDI(isp->priv, 0);
	nsleep(isp->T.su);

	in = 0;
	for (i = 0; i < 8; i++) {
		int val;

		nsleep(isp->T.co);
		isp->SDO(isp->priv, &val);

		in |= (val & 1) << i;

		isp->SCLK(isp->priv, 1);
		nsleep(isp->T.clkh);
		isp->SCLK(isp->priv, 0);
		nsleep(isp->T.clkl);
	}

	*id = in;
	return 0;
}

int ispLSC_Next_State(struct ispLSC *isp)
{
	isp->MODE(isp->priv, 1);
	isp->SDI(isp->priv, 1);

	nsleep(isp->T.su);

	switch (isp->state) {
	case ISP_STATE_IDLE: isp->state = ISP_STATE_SHIFT; break;
	case ISP_STATE_SHIFT: isp->state = ISP_STATE_EXECUTE; break;
	case ISP_STATE_EXECUTE: isp->state = ISP_STATE_SHIFT; break;
	}

	isp->SCLK(isp->priv, 1);
	nsleep(isp->T.h);
	isp->SCLK(isp->priv, 0);
	nsleep(isp->T.clkl);

	isp->MODE(isp->priv, 0);
	isp->SDI(isp->priv, 0);
	nsleep(isp->T.clkh);

	return 0;
}

int ispLSC_Set_Command(struct ispLSC *isp, uint8_t cmd)
{
	int i;

	assert(isp->state = ISP_STATE_SHIFT);

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
	assert(isp->state = ISP_STATE_EXECUTE);

	isp->MODE(isp->priv, 0);
	isp->SDI(isp->priv, 0);

	nsleep(isp->T.su);

	isp->SCLK(isp->priv, 1);
	nsleep(isp->T.clkh);

	isp->SCLK(isp->priv, 0);
}

int ispLSC_Write_Data(struct ispLSC *isp, uint32_t *bitmap, int bits)
{
	int i, v;

	assert(isp->state = ISP_STATE_EXECUTE);

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

	assert(isp->state = ISP_STATE_EXECUTE);

	isp->MODE(isp->priv, 0);

	nsleep(isp->T.su);

	for (i = 0; i < bits; i++) {
		isp->SDO(isp->priv, &v);
		bit_set(bitmap, i, v);

		isp->SCLK(isp->priv, 1);
		nsleep(isp->T.clkh);
		isp->SCLK(isp->priv, 0);
		nsleep(isp->T.co);
	}
}

int ispLSC_Diagnose(struct ispLSC *isp)
{
	int i;
	uint8_t id;
	DECLARE_BITMAP(bitmap, 138);
	DECLARE_BITMAP(verify, 138);

	/* Verify that we can read the device ID */
	ispLSC_Read_ID(isp, &id);
	printf("ID: %d\n", id);

	/* Verify that we can enter command mode */
	ispLSC_Next_State(isp);
	bitmap[0] = 0x19;
	verify[0] = 0;
	ispLSC_Write_Data(isp, bitmap, 5);
	ispLSC_Read_Data(isp, verify, 5);

	printf("WRIT: ");bit_dump(bitmap, 5);printf("\n");
	printf("READ: ");bit_dump(verify, 5);printf("\n");

	if (bitmap[0] != verify[0]) {
		return -EINVAL;
	}

	bitmap[0] = (~0x19) & 0x1f;
	verify[0] = 0;
	ispLSC_Write_Data(isp, bitmap, 5);
	ispLSC_Read_Data(isp, verify, 5);

	printf("WRIT: ");bit_dump(bitmap, 5);printf("\n");
	printf("READ: ");bit_dump(verify, 5);printf("\n");

	if (bitmap[0] != verify[0]) {
		return -EINVAL;
	}

	/* Let's go into arch-shift mode */
	ispLSC_Set_Command(isp, ISPGAL_ARCH_SHIFT);
	ispLSC_Next_State(isp);

	for (i = 0; i < 20; i++) {
		bit_set(&bitmap[0], i, rand() & 1);
	}
	verify[0] = 0;
	ispLSC_Write_Data(isp, bitmap, 20);
	ispLSC_Read_Data(isp, verify, 20);

	printf("WRIT: ");bit_dump(bitmap, 20);printf("\n");
	printf("READ: ");bit_dump(verify, 20);printf("\n");

	/* Go back to SHIFT state */
	ispLSC_Next_State(isp);

	if (bitmap[0] != verify[0]) {
		return -EINVAL;
	}

	/* Let's go into data-shift mode */
	ispLSC_Set_Command(isp, ISPGAL_SHIFT_DATA);
	ispLSC_Next_State(isp);

	for (i = 0; i < 138; i++) {
		bit_set(bitmap, i, rand() & 1);
		bit_set(verify, i, 0);
	}
	ispLSC_Write_Data(isp, bitmap, 138);
	ispLSC_Read_Data(isp, verify, 138);

	printf("WRIT: ");
	for (i = 0; i < 138; i++) {
		printf("%c", '0' + bit_get(bitmap, i));
	}
	printf("\n");

	printf("READ: ");
	for (i = 0; i < 138; i++) {
		printf("%c", '0' + bit_get(verify, i));
	}
	printf("\n");

	/* Go back to SHIFT state */
	ispLSC_Next_State(isp);

	for (i = 0; i < 138; i++) {
		if (bit_get(bitmap, i) != bit_get(verify, i))
			return -EINVAL;
	}

	return 0;
}

