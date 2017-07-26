/*
 * xrick/include/util.h
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

#ifndef UTIL_H
#define UTIL_H

#include "maps.h"

//extern map_flags_t u_envtest(int16_t, int16_t, bool, bool *);
extern map_flags_t u_envtest(int16_t, int16_t, bool);
extern bool u_envtestcrawl(int16_t, int16_t);
extern bool u_isEntitiesColliding(uint8_t, uint8_t);
extern bool u_isEntityCollidePoint(uint8_t, pos_t);
extern bool u_trigbox(uint8_t, pos_t);
extern void u_calc_new_y(uint8_t, int16_t*, uint8_t*);


#endif

/* eof */
