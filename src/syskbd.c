/*
 * xrick/src/syskbd.c
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

#include <SDL.h>

#include "system.h"
syskbd_t syskbd = {
    .up = SDLK_o,
    .down = SDLK_k,
    .left = SDLK_z,
    .right = SDLK_x,
    .pause = SDLK_p,
    .end = SDLK_e,
    .xtra = SDLK_ESCAPE,
    .fire = SDLK_SPACE
};

/* eof */


