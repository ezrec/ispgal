/*
 * Copyright (C) 2010, Jason McMullan <jason.mcmullan@gmail.com>
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

#ifndef ISPGAL_H
#define ISPGAL_H

/* 5-bit ispGAL22V10 commands */
#define ISPGAL_NOP		0x00
#define ISPGAL_SHIFT_DATA	0x02	/* Data Shift register access */
#define ISPGAL_BULK_ERASE	0x03	/* Erase it all */
#define ISPGAL_ERASE_ARRAY	0x05	/* Erase all but the Arch rows */
#define ISPGAL_ERASE_ARCH	0x06	/* Erase just the Arch rows */
#define ISPGAL_PROGRAM		0x07	/* Program Data into the row */
#define ISPGAL_VERIFY		0x0a	/* Load from row into Data */
#define ISPGAL_IOPRLD		0x0d	/* Reload I/O with data */
#define ISPGAL_FLOWTHRU		0x0e	/* Pass through */
#define ISPGAL_ARCH_SHIFT	0x14	/* Enable Arch register */

#define ISPGAL_ID_22V10		0x08

#endif /* ISPGAL_H */
