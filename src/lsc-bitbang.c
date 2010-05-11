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
#include "lsc-bitbang.h"
#include "ispGAL.h"

void nsleep(unsigned int nsec)
{
	struct timespec req = { .tv_sec = 0, .tv_nsec = nsec };
	nanosleep(&req, NULL);
}

static int lsc_Idle(struct lsc_bitbang *lsc)
{
	lsc->MODE(lsc->priv, 1);
	lsc->SDI(lsc->priv, 0);

	nsleep(lsc->T.su);

	lsc->SCLK(lsc->priv, 1);
	nsleep(lsc->T.clkh);
	lsc->SCLK(lsc->priv, 0);
}

static int lsc_Read_ID(struct lsc_bitbang *lsc, uint8_t *id)
{
	int i;
	uint8_t in;

	/* Clock ID into the shift register
	 */
	lsc->MODE(lsc->priv, 1);
	lsc->SDI(lsc->priv, 0);
	nsleep(lsc->T.su);

	lsc->state = LSC_STATE_IDLE;

	for (i = 0; i < 8; i++) {
		lsc->SCLK(lsc->priv, 1);
		nsleep(lsc->T.clkh);
		lsc->SCLK(lsc->priv, 0);
		nsleep(lsc->T.clkl);
	}

	/* Clock shift register (with ID) 
	 * out to SDO
	 */
	lsc->MODE(lsc->priv, 0);
	lsc->SDI(lsc->priv, 0);
	nsleep(lsc->T.su);

	in = 0;
	for (i = 0; i < 8; i++) {
		int val;

		nsleep(lsc->T.co);
		lsc->SDO(lsc->priv, &val);

		in |= (val & 1) << i;

		lsc->SCLK(lsc->priv, 1);
		nsleep(lsc->T.clkh);
		lsc->SCLK(lsc->priv, 0);
		nsleep(lsc->T.clkl);
	}

	*id = in;
	return 0;
}

static int lsc_Next_State(struct lsc_bitbang *lsc)
{
	lsc->MODE(lsc->priv, 1);
	lsc->SDI(lsc->priv, 1);

	nsleep(lsc->T.su);

	switch (lsc->state) {
	case LSC_STATE_IDLE: lsc->state = LSC_STATE_SHIFT; break;
	case LSC_STATE_SHIFT: lsc->state = LSC_STATE_EXECUTE; break;
	case LSC_STATE_EXECUTE: lsc->state = LSC_STATE_SHIFT; break;
	}

	lsc->SCLK(lsc->priv, 1);
	nsleep(lsc->T.h);
	lsc->SCLK(lsc->priv, 0);
	nsleep(lsc->T.clkl);

	lsc->MODE(lsc->priv, 0);
	lsc->SDI(lsc->priv, 0);
	nsleep(lsc->T.clkh);

	return 0;
}

static int lsc_Set_Command(struct lsc_bitbang *lsc, uint8_t cmd)
{
	int i;

	assert(lsc->state = LSC_STATE_SHIFT);

	lsc->MODE(lsc->priv, 0);

	for (i = 0; i < 5; i++){
		lsc->SDI(lsc->priv, (cmd >> i) & 1);
		nsleep(lsc->T.su);
		lsc->SCLK(lsc->priv, 1);
		nsleep(lsc->T.clkh);
		lsc->SCLK(lsc->priv, 0);
	}
}

static int lsc_Run_Command(struct lsc_bitbang *lsc)
{
	assert(lsc->state = LSC_STATE_EXECUTE);

	lsc->MODE(lsc->priv, 0);
	lsc->SDI(lsc->priv, 0);

	nsleep(lsc->T.su);

	lsc->SCLK(lsc->priv, 1);
	nsleep(lsc->T.clkh);

	lsc->SCLK(lsc->priv, 0);
}

static int lsc_Write_Data(struct lsc_bitbang *lsc, const uint32_t *bitmap, int bits)
{
	int i, v;

	assert(lsc->state = LSC_STATE_EXECUTE);

	lsc->MODE(lsc->priv, 0);

	for (i = 0; i < bits; i++) {
		v = bit_get(bitmap, i);

		lsc->SDI(lsc->priv, v);

		nsleep(lsc->T.su);
		lsc->SCLK(lsc->priv, 1);
		nsleep(lsc->T.clkh);
		lsc->SCLK(lsc->priv, 0);
	}
}

static int lsc_Read_Data(struct lsc_bitbang *lsc, uint32_t *bitmap, int bits)
{
	int i, v;

	assert(lsc->state = LSC_STATE_EXECUTE);

	lsc->MODE(lsc->priv, 0);

	nsleep(lsc->T.su);

	for (i = 0; i < bits; i++) {
		lsc->SDO(lsc->priv, &v);
		bit_set(bitmap, i, v);

		lsc->SCLK(lsc->priv, 1);
		nsleep(lsc->T.clkh);
		lsc->SCLK(lsc->priv, 0);
		nsleep(lsc->T.co);
	}
}

int lsc_bitbang_Diagnose(struct lsc *lsc)
{
	struct lsc_bitbang *bb = LSC_PRIV(lsc);
	int i;
	uint8_t id;
	DECLARE_BITMAP(bitmap, 138);
	DECLARE_BITMAP(verify, 138);

	/* Verify that we can read the device ID */
	lsc_Read_ID(bb, &id);
	printf("ID: %d\n", id);

	/* Verify that we can enter command mode */
	lsc_Next_State(bb);
	bitmap[0] = 0x19;
	verify[0] = 0;
	lsc_Write_Data(bb, bitmap, 5);
	lsc_Read_Data(bb, verify, 5);

	printf("WRIT: ");bit_dump(bitmap, 5);printf("\n");
	printf("READ: ");bit_dump(verify, 5);printf("\n");

	if (bitmap[0] != verify[0]) {
		return -EINVAL;
	}

	bitmap[0] = (~0x19) & 0x1f;
	verify[0] = 0;
	lsc_Write_Data(bb, bitmap, 5);
	lsc_Read_Data(bb, verify, 5);

	printf("WRIT: ");bit_dump(bitmap, 5);printf("\n");
	printf("READ: ");bit_dump(verify, 5);printf("\n");

	if (bitmap[0] != verify[0]) {
		return -EINVAL;
	}

	/* Let's go into arch-shift mode */
	lsc_Set_Command(bb, ISPGAL_ARCH_SHIFT);
	lsc_Next_State(bb);

	for (i = 0; i < 20; i++) {
		bit_set(&bitmap[0], i, rand() & 1);
	}
	verify[0] = 0;
	lsc_Write_Data(bb, bitmap, 20);
	lsc_Read_Data(bb, verify, 20);

	printf("WRIT: ");bit_dump(bitmap, 20);printf("\n");
	printf("READ: ");bit_dump(verify, 20);printf("\n");

	/* Go back to SHIFT state */
	lsc_Next_State(bb);

	if (bitmap[0] != verify[0]) {
		return -EINVAL;
	}

	/* Let's go into data-shift mode */
	lsc_Set_Command(bb, ISPGAL_SHIFT_DATA);
	lsc_Next_State(bb);

	for (i = 0; i < 138; i++) {
		bit_set(bitmap, i, rand() & 1);
		bit_set(verify, i, 0);
	}
	lsc_Write_Data(bb, bitmap, 138);
	lsc_Read_Data(bb, verify, 138);

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
	lsc_Next_State(bb);

	for (i = 0; i < 138; i++) {
		if (bit_get(bitmap, i) != bit_get(verify, i))
			return -EINVAL;
	}

	return 0;
}

int lsc_bitbang_Id(struct lsc *lsc, unsigned int *id)
{
	struct lsc_bitbang *bb = LSC_PRIV(lsc);
	uint8_t id8;
	int err;

	err = lsc_Read_ID(bb, &id8);
	if (err < 0)
		return err;

	/* Go to SHIFT state */
	lsc_Next_State(bb);

	*id = id8;
	return err;
}

int lsc_bitbang_Run(struct lsc *lsc, unsigned int cmd, unsigned int nsec)
{
	struct lsc_bitbang *bb = LSC_PRIV(lsc);
	int err;

	err = lsc_Set_Command(bb, cmd);
	if (err < 0) return err;
	err = lsc_Next_State(bb);
	if (err < 0) return err;
	err = lsc_Run_Command(bb);
	if (err < 0) return err;
	bb->nsleep(bb->priv, nsec);
	err = lsc_Next_State(bb);
	if (err < 0) return err;

	return err;
}	

int lsc_bitbang_Read(struct lsc *lsc, unsigned int cmd,
                     unsigned int bits, uint32_t *bitmap)
{
	struct lsc_bitbang *bb = LSC_PRIV(lsc);
	int err;

	err = lsc_Set_Command(bb, cmd);
	if (err < 0) return err;
	err = lsc_Next_State(bb);
	if (err < 0) return err;
	err = lsc_Read_Data(bb, bitmap, bits);
	if (err < 0) return err;
	err = lsc_Next_State(bb);
	if (err < 0) return err;

	return err;
}

int lsc_bitbang_Write(struct lsc *lsc, unsigned int cmd,
                     unsigned int bits, const uint32_t *bitmap)
{
	struct lsc_bitbang *bb = LSC_PRIV(lsc);
	int err;

	err = lsc_Set_Command(bb, cmd);
	if (err < 0) return err;
	err = lsc_Next_State(bb);
	if (err < 0) return err;
	err = lsc_Write_Data(bb, bitmap, bits);
	if (err < 0) return err;
	err = lsc_Next_State(bb);
	if (err < 0) return err;

	return err;
}
