/*
 * xrick/include/control.h
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

#ifndef CONTROL_H
#define CONTROL_H

/* Bit field structure that represent Rick's controls */
typedef struct
{
    bool right:1;
    bool left:1;
    bool down:1;
    bool up:1;
    bool fire:1;
    bool exit:1;
    bool terminate:1;
    bool pause:1;
} control_state;

extern control_state control;
extern bool control_active;

#endif

/* eof */


