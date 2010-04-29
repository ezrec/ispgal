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
#include <stdio.h>
#include <stdlib.h>

#include "jedec.h"
#include "ispLSC.h"
#include "ispGAL.h"

extern int jtagkey_init(struct ispLSC *isp);

static void dump_row_n(struct jedec *jed, int row)
{
	int i;

	printf("%2d: ", row);
	for (i = 0; i < 132; i++) {
		int v, fuse;

		fuse = (5764 + row) - 44*i;

		v = jedec_bit_get(jed, fuse);

		printf("%c", '0' + v);
	}
	printf("\n");
}

static void dump_row_ues(struct jedec *jed)
{
	int i;

	printf("%2d: ", 44);
	for (i = 0; i < 64; i++) {
		int v, fuse;

		fuse = 5891 - i;

		v = jedec_bit_get(jed, fuse);

		printf("%c", '0' + v);
	}
	printf("\n");
}

static void dump_row_asr(struct jedec *jed)
{
	int i;

	printf("ASR ");
	for (i = 0; i < 20; i++) {
		int v, fuse;

		fuse = 5826 - i;
		if (i & 1)
			fuse += 2;

		v = jedec_bit_get(jed, fuse);

		printf("%c", '0' + v);
	}
	printf("\n");
}

static void dump_jed(struct jedec *jed)
{
	int i;
	for (i = 0; i < 44; i++) {
		dump_row_n(jed, i);
	}
	dump_row_ues(jed);
	dump_row_asr(jed);
}

void program_jed(struct jedec *jed)
{
}

extern void nsleep(unsigned int nsec);

int main(int argc, char **argv)
{
	struct jedec *jed;
	struct ispLSC isp;
	uint8_t id;
	uint32_t bitmap[100];
	int i,err;

	err = jtagkey_init(&isp);
	if (err < 0) {
		fprintf(stderr, "Can't initialize FT2232 AmonTek JtagKey\n");
		exit(EXIT_FAILURE);
	}

	isp.T.su   =               100;	/* nsec */
	isp.T.h    =               100;	/* nsec */
	isp.T.co   =               210;	/* nsec */
	isp.T.clkh =               500;	/* nsec */
	isp.T.clkl =               500;	/* nsec */
	isp.T.pwp  =  40 * 1000 * 1000;	/* nsec */
	isp.T.pwe  = 200 * 1000 * 1000; /* nsec */
	isp.T.pwv  =          5 * 1000;	/* nsec */

	ispLSC_Read_ID(&isp, &id);
printf("\n");
	printf("ID: 0x%02x\n", id);
	exit(0);
	ispLSC_Idle(&isp);
printf("\n");
	ispLSC_Next_State(&isp);	/* IDLE => SHIFT */
printf("\n");
	ispLSC_Set_Command(&isp, ISPGAL_SHIFT_DATA);
printf("\n");
	ispLSC_Next_State(&isp);	/* SHIFT => EXECUTE */
printf("\n");
	for (i = 0; i < 132; i++) {
		bit_set(&bitmap, i, 0);
	}
	for (i = 0; i < 6; i++) {
		bit_set(&bitmap, i, 0);
	}
	ispLSC_Write_Data(&isp, bitmap, 138);
printf("\n");
	ispLSC_Next_State(&isp);	/* EXECUTE => SHIFT */
printf("\n");
	ispLSC_Set_Command(&isp, ISPGAL_VERIFY);
printf("\n");
	ispLSC_Next_State(&isp);	/* SHIFT => EXECUTE */
printf("\n");
	ispLSC_Run_Command(&isp);
printf("\n");
	nsleep(isp.T.pwv);
	ispLSC_Next_State(&isp);	/* EXECUTE => SHIFT */
printf("\n");
	ispLSC_Set_Command(&isp, ISPGAL_SHIFT_DATA);
printf("\n");
	ispLSC_Next_State(&isp);	/* SHIFT => EXECUTE */
printf("\n");
	ispLSC_Read_Data(&isp, bitmap, 138);
printf("\n");


	if (0) {
	jed = jedec_read(0);
	if (0) dump_jed(jed);
	program_jed(jed);
	}

	return 0;
}
