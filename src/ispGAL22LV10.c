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
#include "jtag.h"

struct ispGAL22LV10 {
	struct jtag *jtag;

	struct {
		unsigned int pwp;	/* Program time (nsec) */
		unsigned int pwe;	/* Erase time (nsec) */
		unsigned int pwv;	/* Verify time (nsec) */
	} T;
};

static int ispGAL22LV10_open(struct chip *chip, const char *options, const char *interface)
{
	struct ispGAL22LV10 *priv = CHIP_PRIV(chip);
	uint8_t id;
	int i,err;
	DECLARE_BITMAP(cmd, 5);
	DECLARE_BITMAP(in, 32);
	DECLARE_BITMAP(out, 32);

	priv->jtag = jtag_open(interface);
	if (priv->jtag == NULL)
		return -ENODEV;

	priv->T.pwp  =  40 * 1000 * 1000;	/* nsec */
	priv->T.pwe  = 200 * 1000 * 1000; /* nsec */
	priv->T.pwv  =          5 * 1000;	/* nsec */

	bit_set_u32(cmd, 5, JTAGGAL_IDCODE);
	bit_set_u32(in,  32, ~0);
	bit_set_u32(out, 32, JTAGGAL_ID_22LV10);

	err = jtag_IR(priv->jtag, 5, cmd, NULL);
	if (err < 0)
		return err;

	err = jtag_DR(priv->jtag, 32, in, out);
	if (err < 0)
		return err;

	return 0;
}

void ispGAL22LV10_close(struct chip *chip)
{
	struct ispGAL22LV10 *priv = CHIP_PRIV(chip);

	jtag_close(priv->jtag);
}

static int ispGAL22LV10_erase(struct chip *chip)
{
	struct ispGAL22LV10 *priv = CHIP_PRIV(chip);
	int err;
	DECLARE_BITMAP(cmd, 5);

	bit_set_u32(cmd, 5, JTAGGAL_ROW_ERASE);
	err = jtag_IR(priv->jtag, 5, cmd, NULL);
	if (err < 0)
		return err;

	err = jtag_nsleep(priv->jtag, priv->T.pwe);
	if (err < 0)
		return err;

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

		return 132;
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

		return 132;
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
		return;
	} 

	if (row == 44) {	/* UES row, 138 bits */
		/* Pad with dummy ones */
		for (i = 0; i < 64; i++) {
			int fuse;

			fuse = bit_get(bitmap, 68 + i);
			jedec_bit_set(jed, 5828 + i, fuse);
		}
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


static int ispGAL22LV10_program(struct chip *chip, struct jedec *jed)
{
	struct ispGAL22LV10 *priv = CHIP_PRIV(chip);
	int i, len, err;
	DECLARE_BITMAP(cmd, 5);
	DECLARE_BITMAP(row, 44);
	DECLARE_BITMAP(in, 132);

	err = ispGAL22LV10_erase(chip);
	if (err < 0)
		return err;

	for (i = 0; i < 44; i++) {
		bit_set_u32(cmd, 5, JTAGGAL_ROW_MASK);
		bit_zeroes(row, 44);
		bit_set(row, 43 - i, 1);

		jtag_IR(priv->jtag, 5, cmd, NULL);
		jtag_DR(priv->jtag, 44, row, NULL);

		bit_set_u32(cmd, 5, JTAGGAL_ROW_SHIFT);
		len = get_gal_row(jed, i, in);
		assert(len == 132);

		jtag_IR(priv->jtag, 5, cmd, NULL);
		jtag_DR(priv->jtag, 132, in, NULL);

		bit_set_u32(cmd, 5, JTAGGAL_ROW_PROG);
		jtag_IR(priv->jtag, 5, cmd, NULL);
		jtag_nsleep(priv->jtag, priv->T.pwp);
	}

	/* Program architecture row */
	bit_set_u32(cmd, 5, JTAGGAL_ARCH_ERASE);
	jtag_IR(priv->jtag, 5, cmd, NULL);
	jtag_nsleep(priv->jtag, priv->T.pwp);

	bit_set_u32(cmd, 5, JTAGGAL_ARCH_SHIFT);
	jtag_IR(priv->jtag, 5, cmd, NULL);
	len = get_gal_row(jed, 45, in);
	assert(len == 20);
	jtag_DR(priv->jtag, 20, in, NULL);
	bit_set_u32(cmd, 5, JTAGGAL_ARCH_PROG);
	jtag_IR(priv->jtag, 5, cmd, NULL);
	jtag_nsleep(priv->jtag, priv->T.pwp);

	/* Program USERCODE row */
	bit_set_u32(cmd, 5, JTAGGAL_ROW_SHIFT);
	jtag_IR(priv->jtag, 5, cmd, NULL);
	len = get_gal_row(jed, 44, in);
	assert(len == 132);
	jtag_DR(priv->jtag, 132, in, NULL);
	bit_set_u32(cmd, 5, JTAGGAL_USER_PROG);
	jtag_IR(priv->jtag, 5, cmd, NULL);
	jtag_nsleep(priv->jtag, priv->T.pwp);

	return 0;
}

static int ispGAL22LV10_verify(struct chip *chip, struct jedec *jed)
{
	struct ispGAL22LV10 *priv = CHIP_PRIV(chip);
	uint8_t id;
	int len, err, i;
	DECLARE_BITMAP(cmd, 5);
	DECLARE_BITMAP(row, 44);
	DECLARE_BITMAP(in, 132);
	DECLARE_BITMAP(out, 132);

	bit_ones(in, 132);

	for (i = 0; i < 44; i++) {
		bit_set_u32(cmd, 5, JTAGGAL_ROW_MASK);
		bit_zeroes(row, 44);
		bit_set(row, 43 - i, 1);
		
		jtag_IR(priv->jtag, 5, cmd, NULL);
		jtag_DR(priv->jtag, 44, row, NULL);

		bit_set_u32(cmd, 5, JTAGGAL_ROW_VERIFY);
		jtag_IR(priv->jtag, 5, cmd, NULL);
		jtag_nsleep(priv->jtag, priv->T.pwv);

		len = get_gal_row(jed, i, out);
		assert(len == 132);

		err = jtag_DR(priv->jtag, 132, in, out);
		if (err < 0) {
			return err;
		}
	}

	/* Verify architecture row */
	bit_set_u32(cmd, 5, JTAGGAL_ARCH_SHIFT);
	jtag_IR(priv->jtag, 5, cmd, NULL);
	jtag_nsleep(priv->jtag, priv->T.pwp);

	bit_set_u32(cmd, 5, JTAGGAL_ARCH_SHIFT);
	jtag_IR(priv->jtag, 5, cmd, NULL);

	len = get_gal_row(jed, 45, out);
	assert(len == 20);
	jtag_DR(priv->jtag, 20, in, out);

	/* Verify USERCODE row */
	bit_set_u32(cmd, 5, JTAGGAL_USER_VERIFY);
	jtag_IR(priv->jtag, 5, cmd, NULL);
	jtag_nsleep(priv->jtag, priv->T.pwp);

	bit_set_u32(cmd, 5, JTAGGAL_ROW_SHIFT);
	jtag_IR(priv->jtag, 5, cmd, NULL);

	len = get_gal_row(jed, 44, out);
	assert(len == 132);
	jtag_DR(priv->jtag, 132, in, out);

	return 0;
}

static int ispGAL22LV10_diagnose(struct chip *chip)
{
	return 0;
}

static int ispGAL22LV10_prog_enter(struct ispGAL22LV10 *priv)
{
	DECLARE_BITMAP(cmd, 5);

	bit_set_u32(cmd, 5, JTAGGAL_PROG_ENABLE);
	jtag_IR(priv->jtag, 5, cmd, NULL);
	jtag_nsleep(priv->jtag, priv->T.pwp);
}

static int ispGAL22LV10_prog_exit(struct ispGAL22LV10 *priv)
{
	DECLARE_BITMAP(cmd, 5);

	bit_set_u32(cmd, 5, JTAGGAL_PROG_DISABLE);
	jtag_IR(priv->jtag, 5, cmd, NULL);
}

static int ispGAL22LV10_wrap_erase(struct chip *chip)
{
	struct ispGAL22LV10 *priv = CHIP_PRIV(chip);
	int err;

	ispGAL22LV10_prog_enter(priv);
	err = ispGAL22LV10_erase(chip);
	ispGAL22LV10_prog_exit(priv);

	return err;
}

static int ispGAL22LV10_wrap_program(struct chip *chip, struct jedec *jed)
{
	struct ispGAL22LV10 *priv = CHIP_PRIV(chip);
	int err;

	ispGAL22LV10_prog_enter(priv);
	err = ispGAL22LV10_program(chip, jed);
	ispGAL22LV10_prog_exit(priv);

	return err;
}

static int ispGAL22LV10_wrap_verify(struct chip *chip, struct jedec *jed)
{
	struct ispGAL22LV10 *priv = CHIP_PRIV(chip);
	int err;

	ispGAL22LV10_prog_enter(priv);
	err = ispGAL22LV10_verify(chip, jed);
	ispGAL22LV10_prog_exit(priv);

	return err;
}

struct chip chip_ispGAL22LV10 = {
	.name   = "ispGAL22LV10",
	.open	= ispGAL22LV10_open,
	.close	= ispGAL22LV10_close,
	.erase	= ispGAL22LV10_wrap_erase,
	.program = ispGAL22LV10_wrap_program,
	.verify	= ispGAL22LV10_wrap_verify,
	.diagnose = ispGAL22LV10_diagnose,
	.priv_size = sizeof(struct ispGAL22LV10),
};

