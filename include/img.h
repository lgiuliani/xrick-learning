/*
 * xrick/include/img.h
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

#ifndef IMG_H
#define IMG_H

typedef struct
{
    uint8_t r, g, b, nothing;
} img_color_t;

typedef struct
{
    uint16_t w, h;
    uint16_t ncolors;
    img_color_t *colors;
    uint8_t *pixels;
} img_t;

#endif

/* eof */
