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
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "jedec.h"
#include "chip.h"

#ifndef true
#define false	(0)
#define true	(!false)
#endif

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

static void usage(const char *program)
{
	fprintf(stderr, "Usage:\n\n"
			"%s [options] check\n"
			"%s [options] erase\n"
			"%s [options] program <filename.jed>\n"
			"%s [options] verify  <filename.jed>\n"
			"\n"
			"  -c, --chip=type[,options] Chip type (\"help\" lists all chips\n"
			"  -t, --tool=type[,options] Programming tool (\"help\" lists all tools for\n"
			"                            the selected chip type.\n"

			"  -h, -?, --help            This help.\n"
			"\n"
			"  check                     Check that the chip is operational\n"
			"  erase                     Erase the chip\n"
			"  program                   Program the chip with the JEDEC fuse map\n"
			"  verify                    Verify the chip against the JEDEC fuse map\n"
			, program, program, program, program);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	struct jedec *jed;
	struct chip *chip;
	int err;
	int c;
	const char *jedec = NULL;
	const char *chip_name = NULL;
	const char *tool_name = NULL;
	int option;
	enum {
		MODE_INVALID = 0,
		MODE_CHECK,
		MODE_ERASE,
		MODE_PROGRAM,
		MODE_VERIFY,
	} mode;
	const struct option options[] = {
		{ .name = "chip",   .has_arg = true, .flag = NULL, .val = 'c' },
		{ .name = "tool",   .has_arg = true, .flag = NULL, .val = 't' },
		{ .name = "help",   .has_arg = false, .flag = NULL, .val = 'h' },
		{ .name = NULL },
	};

	while ((c = getopt_long(argc, argv, "+c:t:h", options, &option)) >= 0) {
		switch (c) {
		case 'c':
			chip_name = optarg;
			break;
		case 't':
			tool_name = optarg;
			break;
		default:
			usage(argv[0]);
			break;
		}
	}

	mode = MODE_INVALID;
	if (optind == argc) {
		mode = MODE_CHECK;
	} else if ((argc - optind) == 1) {
		if (strcmp(argv[optind], "check") == 0) {
			mode = MODE_CHECK;
		} else if (strcmp(argv[optind], "erase") == 0) {
			mode = MODE_ERASE;
		}
	} else if ((argc - optind) == 2) {
		if (strcmp(argv[optind], "program") == 0) {
			mode = MODE_PROGRAM;
			jedec = argv[optind + 1];
		} else if (strcmp(argv[optind], "verify") == 0) {
			mode = MODE_VERIFY;
			jedec = argv[optind + 1];
		}
	}
	if (mode == MODE_INVALID)
		usage(argv[0]);

	chip = chip_open(chip_name, tool_name);
	assert(chip != NULL);

	fprintf(stderr, "%s:%s Check...\n", chip_name, tool_name);
	err = chip_diagnose(chip);
	if (err < 0) {
		fprintf(stderr, "%s:%s Functionality check failed\n", chip_name, tool_name);
		chip_close(chip);
		return EXIT_FAILURE;
	}

	if (mode == MODE_CHECK) {
		chip_close(chip);
		return EXIT_SUCCESS;
	}

	/* If we're not going to erase,
	 * load the JEDEC fuse map
	 */
	if (mode != MODE_ERASE) {
		if (jedec == NULL) {
			fprintf(stderr, "%s:%s No JEDEC fuse map specified for this operation\n", chip_name, tool_name);
			chip_close(chip);
			return EXIT_FAILURE;
		}
		if (strcmp(jedec, "-") == 0)
			jedec = "/dev/stdin";

		err = open(jedec, O_RDONLY);
		if (err < 0) {
			fprintf(stderr, "%s: %s\n", jedec, strerror(errno));
			chip_close(chip);
			return EXIT_FAILURE;
		}
		jed = jedec_read(err);
		close(err);
		if (jed == NULL) {
			fprintf(stderr, "%s: Invalid JEDEC file\n", jedec);
			chip_close(chip);
			return EXIT_FAILURE;
		}
	}

	if (mode == MODE_VERIFY)
		goto verify;

	fprintf(stderr, "%s:%s Erase...\n", chip_name, tool_name);
	err = chip_erase(chip);
	if (err < 0) {
		fprintf(stderr, "%s:%s Erase failed\n", chip_name, tool_name);
		chip_close(chip);
		return EXIT_FAILURE;
	}

	if (mode == MODE_ERASE) {
		chip_close(chip);
		return EXIT_SUCCESS;
	}

	fprintf(stderr, "%s:%s Program...\n", chip_name, tool_name);
	err = chip_program(chip, jed);
	if (err < 0) {
		fprintf(stderr, "%s:%s Programming failed\n", chip_name, tool_name);
		chip_close(chip);
		return EXIT_FAILURE;
	}

verify:
	fprintf(stderr, "%s:%s Verify...\n", chip_name, tool_name);
	err = chip_verify(chip, jed);
	if (err < 0) {
		fprintf(stderr, "%s:%s Verify failed\n", chip_name, tool_name);
		chip_close(chip);
		return EXIT_FAILURE;
	}

	chip_close(chip);
	return 0;
}
