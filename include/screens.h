/*
 * xrick/include/screens.h
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

#ifndef SCREENS_H
#define SCREENS_H

#define SCREEN_TIMEOUT 4000

typedef struct
{
    uint16_t count;  /* number of loops */
    uint16_t dx, dy;  /* sprite x and y deltas */
    uint16_t base;  /* base for sprite numbers table */
} screen_imapsteps_t;  /* description of one step */

extern const uint8_t screen_imapsl[];  /* sprite lists */
extern const screen_imapsteps_t screen_imapsteps[];  /* map intro steps */
extern const uint8_t screen_imapsofs[];  /* first step for each map */
extern uint8_t const* const screen_imaptext[];  /* map intro texts */
extern const uint8_t screen_imainhoft[];  /* hall of fame title */
extern const uint8_t screen_imainrdt[];  /*rick dangerous title */
extern const uint8_t screen_imaincdc[];  /* core design copyright text */
extern const uint8_t screen_gameovertxt[];  /* game over */
extern const uint8_t screen_pausedtxt[];  /* paused */
extern const uint8_t screen_congrats[];  /* congratulations */

extern state_t screen_introMain(void);  /* main intro */
extern state_t screen_introMap(void);  /* map intro */
extern state_t screen_gameover(void);  /* gameover */
extern state_t screen_getname(void);  /* enter you name */
extern void screen_pause(bool);  /* pause indicator */

#endif

/* eof */
