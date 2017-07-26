/*
 * xrick/include/rects.h
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

#ifndef RECTS_H
#define RECTS_H
#include <stdint.h>

typedef struct rect_s
{
    uint16_t x, y;
    uint16_t width, height;
    struct rect_s *next;
} rect_t;

extern void rects_free(rect_t *);
extern rect_t *rects_new(uint16_t, uint16_t, uint16_t, uint16_t, rect_t *);

#endif

/* eof */
