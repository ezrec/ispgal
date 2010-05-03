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
#include <string.h>

#include "bitmap.h"
#include "jtag.h"


struct svf {
	FILE *out;
};

static int svf_open(struct jtag *jtag, const char *options)
{
	struct svf *svf = JTAG_PRIV(jtag);

	svf->out = stdout;
	if (options != NULL) {
		svf->out = fopen(options, "w");
		if (svf->out == NULL) {
			fprintf(stderr, "%s: %s\n", options, strerror(errno));
		}
	}

	fprintf(svf->out, "! ispgal generated SVF file\r\n");
	fprintf(svf->out, "HDR 0;\r\n");
	fprintf(svf->out, "HIR 0;\r\n");
	fprintf(svf->out, "TDR 0;\r\n");
	fprintf(svf->out, "TIR 0;\r\n");
	fprintf(svf->out, "ENDDR DRPAUSE;\r\n");
	fprintf(svf->out, "ENDIR IRPAUSE;\r\n");
	fprintf(svf->out, "FREQUENCY 1000000 HZ;\r\n");

	return 0;
}

static void svf_close(struct jtag *jtag)
{
	struct svf *svf = JTAG_PRIV(jtag);

	fclose(svf->out);
}

static int svf_nsleep(struct jtag *jtag, unsigned int nsec)
{
	struct svf *svf = JTAG_PRIV(jtag);

	fprintf(svf->out, "RUNTEST IDLE 3 TCK %uE-9 SEC;\r\n", nsec);
	return 0;
}

static void svf_hexdump(FILE *out, uint32_t *mask, int bits)
{
	uint8_t val = 0;
	int i, bitmax, offset;
	const char hexmap[] = "0123456789ABCDEF";

	bitmax = (bits + 3) & ~3;

	/* Catch leading zeros */
	for (i = bitmax - 1; i >= 0; i--) {
		if (i >= bits)
			continue;
		offset = (i & 3);
		val |= bit_get(mask, i) << offset;
		if (offset == 0) {
			fprintf(out, "%c", hexmap[val]);
			val = 0;
		}
	}
}

static int svf_cmd(struct svf *svf, const char *name, unsigned int bits, uint32_t *in, uint32_t *out)
{
	fprintf(svf->out, "%s %d", name, bits);
	if (in != NULL) {
		fprintf(svf->out, " TDI (");
		svf_hexdump(svf->out, in, bits);
		fprintf(svf->out, ")");
	}
	if (in != NULL && out != NULL) {
		fprintf(svf->out, "\r\n\t");
	}
	if (out != NULL) {
		fprintf(svf->out, " TDO (");
		svf_hexdump(svf->out, out, bits);
		fprintf(svf->out, ")");
	}
	fprintf(svf->out, ";\r\n");

	return 0;
}

static int svf_IR(struct jtag *jtag, unsigned int bits, uint32_t *in, uint32_t *out)
{
	struct svf *svf = JTAG_PRIV(jtag);
	return svf_cmd(svf, "SIR", bits, in, out);
}

static int svf_DR(struct jtag *jtag, unsigned int bits, uint32_t *in, uint32_t *out)
{
	struct svf *svf = JTAG_PRIV(jtag);
	return svf_cmd(svf, "SDR", bits, in, out);
}

const struct jtag jtag_svf = {
	.name = "svf",
	.help = "\tsvf,filename.svf     Output filename (if not stdout)\n",
	.open = svf_open,
	.close = svf_close,
	.nsleep = svf_nsleep,
	.IR = svf_IR,
	.DR = svf_DR,

	.priv_size = sizeof(struct svf),
};
