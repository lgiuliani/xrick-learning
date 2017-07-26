/*
 * xrick/include/tiles.h
 *
 * Copyright (C) 1998-2002 BigOrno (bigorno@bigorno.net). All rights reserved.
 *
 * The use and distribution terms for this software are contained in the file
 * named README, which can be found in the root of this distribution. By
 * using this software in any fashion, you are agreeing to be bound by the
 * terms of this license.
 *
 * You must not remove this notice, or any other, from this software.
 */

/*
 * NOTES
 *
 * A tile consists in one column and 8 rows of 8 uint16_t (cga encoding, two
 * bits per pixel). The tl_tiles array contains all tiles, with the
 * following structure:
 *
 *  0x0000 - 0x00FF  tiles for main intro
 *  0x0100 - 0x01FF  tiles for map intro
 *  0x0200 - 0x0327  unused
 *  0x0328 - 0x0427  game tiles, page 0
 *  0x0428 - 0x0527  game tiles, page 1
 *  0x0527 - 0x05FF  unused
 */

#ifndef TILES_H
#define TILES_H

#define TILES_NBR_BANKS 3

/*
#define TILES_SIZEOF8 (0x10)
#define TILES_SIZEOF16 (0x08)
*/

/*
 * one single tile
 */
typedef uint32_t tile_t[0x08];

/*
 * tiles banks (each bank is 0x100 tiles)
 */
extern const tile_t tiles_data[TILES_NBR_BANKS][0x100];

#endif

/* eof */
