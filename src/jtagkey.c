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

#include <errno.h>
#include <stdio.h>

#include <ftdi.h>

#include "jtag.h"
#include "lsc.h"
#include "lsc-bitbang.h"

#define PIN_SCLK	(1 << 0)
#define PIN_SDI		(1 << 1)
#define PIN_SDO		(1 << 2)
#define PIN_MODE	(1 << 3)

struct jtagkey {
	struct ftdi_context ftdi;
	unsigned char pins;
};

static int jtagkey_MODE(void *priv, int v)
{
	struct jtagkey *jk = priv;

	jk->pins = (jk->pins & ~PIN_MODE) | (v ? PIN_MODE : 0);

	return ftdi_write_data(&jk->ftdi, &jk->pins, 1);
}

static int jtagkey_SDI(void *priv, int v)
{
	struct jtagkey *jk = priv;

	jk->pins = (jk->pins & ~PIN_SDI) | (v ? PIN_SDI : 0);

	return ftdi_write_data(&jk->ftdi, &jk->pins, 1);
}

static int jtagkey_SCLK(void *priv, int v)
{
	struct jtagkey *jk = priv;

	jk->pins = (jk->pins & ~PIN_SCLK) | (v ? PIN_SCLK : 0);

	return ftdi_write_data(&jk->ftdi, &jk->pins, 1);
}

static int jtagkey_SDO(void *priv, int *v)
{
	struct jtagkey *jk = priv;
	unsigned char c;
	int err;

	err = ftdi_read_pins(&jk->ftdi, &c);
	if (err < 0) {
		return -EIO;
	}

	*v = (c & PIN_SDO) ? 1 : 0;
	return 0;
}

static int jtagkey_open(struct jtagkey *jk, const char *flags)
{
	uint8_t buff[3];
	uint32_t bytes;
	int err;
	const int vid = 0x0403, pid = 0xcff8;

	err = ftdi_init(&jk->ftdi);
	if (err < 0)
		return -ENODEV;

	err = ftdi_usb_open_desc(&jk->ftdi, vid, pid, NULL, NULL);
	if (err < 0)
		return -ENODEV;

	err = ftdi_usb_reset(&jk->ftdi);
	if (err < 0)
		return -ENODEV;

#if 0
	err = ftdi_set_interface(&jk->ftdi, INTERFACE_A);
	if (err < 0)
		return -ENODEV;
#endif

	/* Set speed */
	err = ftdi_set_baudrate(&jk->ftdi, 115200 * 12);
	if (err < 0)
		return -ENODEV;

	ftdi_set_bitmode(&jk->ftdi, PIN_MODE | PIN_SDI | PIN_SCLK, BITMODE_BITBANG);
	return 0;
}

/****** LSC mode ********/
void jtagkey_nsleep(struct jtagkey *jk, unsigned long nsec)
{
	nsleep(nsec);
}

int jtagkey_lsc_open(struct lsc *lsc, const char *flags)
{
	struct lsc_bitbang *bb = LSC_PRIV(lsc);
	struct jtagkey *jk = (void *)(&bb[1]);
	int err;

	err = jtagkey_open(jk, flags);
	if (err < 0)
		return err;

	bb->priv   = jk;
	bb->MODE   = jtagkey_MODE;
	bb->SDI    = jtagkey_SDI;
	bb->SCLK   = jtagkey_SCLK;
	bb->SDO    = jtagkey_SDO;
	bb->nsleep = jtagkey_nsleep;
	bb->T.su   = 100;	/* nsec */
	bb->T.h    = 100;	/* nsec */
	bb->T.co   = 210;	/* nsec */
	bb->T.clkh = 500;	/* nsec */
	bb->T.clkl = 500;	/* nsec */

	return 0;
}

void jtagkey_lsc_close(struct lsc *lsc)
{
}

struct lsc lsc_jtagkey = {
	.name = "jtagkey",
	.help = NULL,
	.open = jtagkey_lsc_open,
	.close = jtagkey_lsc_close,

	.Id = lsc_bitbang_Id,
	.Read = lsc_bitbang_Read,
	.Write = lsc_bitbang_Write,
	.Run = lsc_bitbang_Run,

	.priv_size = sizeof(struct lsc_bitbang) + sizeof(struct jtagkey),
};


/****** JTAG mode *******/
int jtagkey_jtag_open(struct jtag *jtag)
{
	return 0;
}

void jtagkey_jtag_close(struct jtag *jtag)
{
}

int jtagkey_jtag_nsleep(struct jtag *jtag, unsigned int nsec)
{
	return 0;
}

int jtagkey_jtag_IR(struct jtag *jtag, unsigned int bits, uint32_t *in, uint32_t *out)
{
	return 0;
}

int jtagkey_jtag_DR(struct jtag *jtag, unsigned int bits, uint32_t *in, uint32_t *out)
{
	return 0;
}

const struct jtag jtag_jtagkey = {
	.name = "jtagkey",
	.open = jtagkey_jtag_open,
	.close = jtagkey_jtag_close,
	.nsleep = jtagkey_jtag_nsleep,
	.IR = jtagkey_jtag_IR,
	.DR = jtagkey_jtag_DR,

	.priv_size = 0,
};
