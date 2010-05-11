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
#include <stdlib.h>

#include "chip.h"
#include "ispGAL.h"
#include "lsc.h"

struct ispGAL22V10 {
	struct lsc *lsc;
	struct {
		unsigned int pwe;	/* nsec */
		unsigned int pwp;	/* nsec */
		unsigned int pwv;	/* nsec */
	} T;
};

static int ispGAL22V10_open(struct chip *chip, const char *options, const char *tool)
{
	struct ispGAL22V10 *priv = CHIP_PRIV(chip);
	unsigned int id;
	uint32_t bitmap[100];
	int i,err;

	priv->lsc = lsc_open(tool);
	if (priv->lsc == NULL)
		return -ENODEV;

	priv->T.pwp  =  40 * 1000 * 1000;	/* nsec */
	priv->T.pwe  = 200 * 1000 * 1000; /* nsec */
	priv->T.pwv  =          5 * 1000;	/* nsec */

	lsc_Id(priv->lsc, &id);

	if (id != 0x08) {
		fprintf(stderr, "ispGAL22V10 ID expected 0x08, read back 0x%02x\n", id);
		return -ENODEV;
	}

	return 0;
}

static void ispGAL22V10_close(struct chip *chip)
{
	struct ispGAL22V10 *priv = CHIP_PRIV(chip);

	lsc_close(priv->lsc);
}

static int ispGAL22V10_erase(struct chip *chip)
{
	struct ispGAL22V10 *priv = CHIP_PRIV(chip);
	struct jedec *jed;

	return lsc_Run(priv->lsc, ISPGAL_BULK_ERASE, priv->T.pwe);
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
		for (i = 0; i < 64; i++) {
			int fuse;

			fuse = jedec_bit_get(jed, 5828 + i);
			bit_set(bitmap, i, fuse);
		}

		/* Pad with dummy ones */
		for (i = 0; i < 68; i++) {
			bit_set(bitmap, 64 + i, 1);
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
		for (i = 0; i < 64; i++) {
			int fuse;

			fuse = bit_get(bitmap, i);
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
		len = get_gal_row(jed, i, &bitmap[0]);

		if (i == 45)
			lsc_Write(priv->lsc, ISPGAL_ARCH_SHIFT, len, &bitmap[0]);
		else
			lsc_Write(priv->lsc, ISPGAL_SHIFT_DATA, len, &bitmap[0]);

		lsc_Run(priv->lsc, ISPGAL_PROGRAM, priv->T.pwp);
	}

	return 0;
}

static int ispGAL22V10_verify(struct chip *chip, struct jedec *jed)
{
	struct ispGAL22V10 *priv = CHIP_PRIV(chip);
	int len, i;
	unsigned int cmd;
	DECLARE_BITMAP(bitmap, 138);

	for (i = 0; i < 46; i++) {
		len = get_gal_row(jed, i, &bitmap[0]);

		if (i == 45)
			cmd = ISPGAL_ARCH_SHIFT;
		else
			cmd = ISPGAL_SHIFT_DATA;

		lsc_Write(priv->lsc, cmd, len, &bitmap[0]);
		lsc_Run(priv->lsc, ISPGAL_VERIFY, priv->T.pwv);
		lsc_Read(priv->lsc, cmd, len, &bitmap[0]);

		put_gal_row(jed, i, &bitmap[0]);
	}

	return 0;
}

static int ispGAL22V10_diagnose(struct chip *chip)
{
	struct ispGAL22V10 *priv = CHIP_PRIV(chip);

	return lsc_bitbang_Diagnose(priv->lsc);
}

struct chip chip_ispGAL22V10 = {
	.name   = "ispGAL22V10",
	.open	= ispGAL22V10_open,
	.close	= ispGAL22V10_close,
	.erase	= ispGAL22V10_erase,
	.program = ispGAL22V10_program,
	.verify	= ispGAL22V10_verify,
	.diagnose = ispGAL22V10_diagnose,
	.priv_size = sizeof(struct ispGAL22V10)
};

