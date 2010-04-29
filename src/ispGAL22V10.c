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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "chip.h"
#include "ispGAL.h"
#include "ispLSC.h"

struct ispGAL22V10 {
	struct ispLSC isp;
};

static int ispGAL22V10_init(struct chip *chip, const char *interface)
{
	struct ispGAL22V10 *priv = CHIP_PRIV(chip);
	uint8_t id;
	uint32_t bitmap[100];
	int i,err;

	if (strcasecmp(interface, "jtagkey") != 0) {
		return -EINVAL;
	}

	err = jtagkey_init(&priv->isp);
	if (err < 0) {
		fprintf(stderr, "Can't initialize FT2232 AmonTek JtagKey\n");
		exit(EXIT_FAILURE);
	}

	priv->isp.T.su   =               100;	/* nsec */
	priv->isp.T.h    =               100;	/* nsec */
	priv->isp.T.co   =               210;	/* nsec */
	priv->isp.T.clkh =               500;	/* nsec */
	priv->isp.T.clkl =               500;	/* nsec */
	priv->isp.T.pwp  =  40 * 1000 * 1000;	/* nsec */
	priv->isp.T.pwe  = 200 * 1000 * 1000; /* nsec */
	priv->isp.T.pwv  =          5 * 1000;	/* nsec */

	ispLSC_Read_ID(&priv->isp, &id);

	if (id != 0x08) {
		fprintf(stderr, "ispGAL22V10 ID expected 0x08, read back 0x%02x\n", id);
		return -ENODEV;
	}

	return 0;
}

static int ispGAL22V10_erase(struct chip *chip)
{
	struct ispGAL22V10 *priv = CHIP_PRIV(chip);
	struct jedec *jed;
	uint8_t id;

	ispLSC_Read_ID(&priv->isp, &id);
	if (id != 0x08)
		return -ENODEV;

	ispLSC_Next_State(&priv->isp);	/* IDLE => SHIFT */
	ispLSC_Set_Command(&priv->isp, ISPGAL_BULK_ERASE);
	ispLSC_Next_State(&priv->isp);	/* SHIFT => EXECUTE */
	ispLSC_Run_Command(&priv->isp);

	nsleep(priv->isp.T.pwe);

	ispLSC_Next_State(&priv->isp);	/* EXECUTE => SHIFT */

	return 0;
}

/* Copy row from JEDEC to bitmap
 */
static int get_gal_row(struct jedec *jed, int row, uint32_t *bitmap)
{
	int i;

	if (row >= 0 && row < 44) {
		/* AND array row, 138 bits */
		for (i = 0; i < 132; i++) {
			int fuse;

			fuse = jedec_bit_get(jed, (i * 44) + row);
			bit_set(bitmap, i, fuse);
		}

		for (i = 0; i < 6; i++) {
			bit_set(bitmap, 132 + i, (row >> i) & 1);
		}

		return 138;
	} 

	if (row == 44) {	/* UES row, 138 bits */
		/* Pad with dummy ones */
		for (i = 0; i < 68; i++) {
			bit_set(bitmap, i, 1);
		}
		for (i = 0; i < 64; i++) {
			int fuse;

			fuse = jedec_bit_get(jed, 5828 + i);
			bit_set(bitmap, 68 + i, fuse);
		}

		for (i = 0; i < 6; i++) {
			bit_set(bitmap, 132 + i, (row >> i) & 1);
		}

		return 138;
	}

	if (row == 45) {	/* Architecture row, 20 bits */
		for (i = 0; i < 20; i++) {
			int fuse;

			fuse = jedec_bit_get(jed, 5808 + (i ^ 1));
			bit_set(bitmap, i, fuse);
		}

		return 20;
	}

	return -EINVAL;
}

/* Copy row from bitmap to JEDEC
 */
static void put_gal_row(struct jedec *jed, int row, uint32_t *bitmap)
{
	int i;
	int rid;

	if (row >= 0 && row < 44) {
		/* AND array row, 138 bits */
		for (i = 0; i < 132; i++) {
			int fuse;

			fuse = bit_get(bitmap, i);
			jedec_bit_set(jed, (i * 44) + row, fuse);
		}

		rid = 0;
		for (i = 0; i < 6; i++) {
			rid |= (bit_get(bitmap, 132 + i) << i);
		}

		assert(rid == row);

		return;
	} 

	if (row == 44) {	/* UES row, 138 bits */
		/* Pad with dummy ones */
		for (i = 0; i < 64; i++) {
			int fuse;

			fuse = bit_get(bitmap, 68 + i);
			jedec_bit_set(jed, 5828 + i, fuse);
		}

		rid = 0;
		for (i = 0; i < 6; i++) {
			rid |= bit_get(bitmap, 132 + i) << i;
		}

		assert(row == rid);

		return;
	}

	if (row == 45) {	/* Architecture row, 20 bits */
		for (i = 0; i < 20; i++) {
			int fuse;

			fuse = bit_get(bitmap, i);
			jedec_bit_set(jed, 5808 + (i ^ 1), fuse);
		}

		return;
	}

	return;
}


static int ispGAL22V10_program(struct chip *chip, struct jedec *jed)
{
	struct ispGAL22V10 *priv = CHIP_PRIV(chip);
	int i, len, err;
	DECLARE_BITMAP(bitmap, 138);

	err = ispGAL22V10_erase(chip);
	if (err < 0)
		return err;

	for (i = 0; i < 46; i++) {
		if (i == 45)
			ispLSC_Set_Command(&priv->isp, ISPGAL_ARCH_SHIFT);
		else
			ispLSC_Set_Command(&priv->isp, ISPGAL_SHIFT_DATA);
		ispLSC_Next_State(&priv->isp);	/* SHIFT => EXECUTE */

		len = get_gal_row(jed, i, &bitmap[0]);

		ispLSC_Write_Data(&priv->isp, &bitmap[0], len);
		ispLSC_Next_State(&priv->isp);	/* EXECUTE => SHIFT */

		ispLSC_Set_Command(&priv->isp, ISPGAL_PROGRAM);
		ispLSC_Next_State(&priv->isp);	/* SHIFT => EXECUTE */

		ispLSC_Run_Command(&priv->isp);
		nsleep(priv->isp.T.pwp);
		ispLSC_Next_State(&priv->isp);	/* EXECUTE => SHIFT */
	}

	return 0;
}

static int ispGAL22V10_verify(struct chip *chip, struct jedec *jed)
{
	struct ispGAL22V10 *priv = CHIP_PRIV(chip);
	uint8_t id;
	int len, i;
	DECLARE_BITMAP(bitmap, 138);

	ispLSC_Read_ID(&priv->isp, &id);
	if (id != 0x08)
		return -ENODEV;

	for (i = 0; i < 46; i++) {
		if (i == 45)
			ispLSC_Set_Command(&priv->isp, ISPGAL_ARCH_SHIFT);
		else
			ispLSC_Set_Command(&priv->isp, ISPGAL_SHIFT_DATA);
		ispLSC_Next_State(&priv->isp);	/* SHIFT => EXECUTE */

		len = get_gal_row(jed, i, &bitmap[0]);

		ispLSC_Write_Data(&priv->isp, &bitmap[0], len);
		ispLSC_Next_State(&priv->isp);	/* EXECUTE => SHIFT */

		ispLSC_Set_Command(&priv->isp, ISPGAL_VERIFY);
		ispLSC_Next_State(&priv->isp);	/* SHIFT => EXECUTE */

		ispLSC_Run_Command(&priv->isp);
		nsleep(priv->isp.T.pwv);
		ispLSC_Next_State(&priv->isp);	/* EXECUTE => SHIFT */

		if (i == 45)
			ispLSC_Set_Command(&priv->isp, ISPGAL_ARCH_SHIFT);
		else
			ispLSC_Set_Command(&priv->isp, ISPGAL_SHIFT_DATA);
		ispLSC_Next_State(&priv->isp);	/* SHIFT => EXECUTE */

		ispLSC_Read_Data(&priv->isp, &bitmap[0], len);
		put_gal_row(jed, i, &bitmap[0]);
		ispLSC_Next_State(&priv->isp);	/* EXECUTE => SHIFT */
	}

	return 0;
}

struct chip chip_ispGAL22V10 = {
	.init	= ispGAL22V10_init,
	.erase	= ispGAL22V10_erase,
	.program = ispGAL22V10_program,
	.verify	= ispGAL22V10_verify,
	.priv_size = sizeof(struct ispGAL22V10)
};

