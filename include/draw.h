/*
 * xrick/include/draw.h
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

#ifndef DRAW_H
#define DRAW_H

#include "rects.h"
#include "img.h"

extern uint8_t draw_tilesBank;
extern rect_t draw_STATUSRECT;
extern rect_t draw_SCREENRECT;

extern void draw_setfb(uint16_t, uint16_t);
extern bool draw_isOutsideScreen(pos_t, dim_t);
extern void draw_clipToMapScreen(pos_t *, dim_t *);
extern void draw_tilesList(uint8_t const *);
extern void draw_tilesSubList(uint8_t const *);
extern void draw_tile(uint8_t);
extern void draw_sprite(uint8_t);
extern void draw_sprite2(uint8_t, pos_t, dim_t, bool);
extern void draw_spriteBackground(pos_t);
extern void draw_map(void);
extern void draw_drawStatus(void);
extern void draw_clearStatus(void);
extern void draw_pic(uint16_t, uint32_t const *);
extern void draw_infos(void);
extern void draw_gettoscreen(pos_t *);

#endif

/* eof */
