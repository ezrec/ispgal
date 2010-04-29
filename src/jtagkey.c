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

#include "ispLSC.h"

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
printf("%c", (jk->pins & PIN_MODE) ? 'M' : 'm');

	return ftdi_write_data(&jk->ftdi, &jk->pins, 1);
}

static int jtagkey_SDI(void *priv, int v)
{
	struct jtagkey *jk = priv;

	jk->pins = (jk->pins & ~PIN_SDI) | (v ? PIN_SDI : 0);
printf("%c", (jk->pins & PIN_SDI) ? 'I' : 'i');

	return ftdi_write_data(&jk->ftdi, &jk->pins, 1);
}

static int jtagkey_SCLK(void *priv, int v)
{
	struct jtagkey *jk = priv;

	jk->pins = (jk->pins & ~PIN_SCLK) | (v ? PIN_SCLK : 0);
printf("%c", (jk->pins & PIN_SCLK) ? '^' : 'v');

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
printf("%x", c & 0xf);
printf("%c", (c & PIN_SDO) ? '1' : '0');

	*v = (c & PIN_SDO) ? 1 : 0;
	return 0;
}

int jtagkey_init(struct ispLSC *isp)
{
	uint8_t buff[3];
	uint32_t bytes;
	int err;
	struct jtagkey *jk;
	const int vid = 0x0403, pid = 0xcff8;

	jk = malloc(sizeof(*jk));

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

	isp->priv = jk;
	isp->MODE = jtagkey_MODE;
	isp->SDI = jtagkey_SDI;
	isp->SCLK = jtagkey_SCLK;
	isp->SDO = jtagkey_SDO;

	return 0;
}