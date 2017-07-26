/*
 * xrick/include/game.h
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

#ifndef GAME_H
#define GAME_H

#include "config.h"

#include "rects.h"
#include "data.h"

#define GAME_PERIOD 79
#define GAME_BOMBS_INIT 6
#define GAME_BULLETS_INIT 6
#define GAME_LIVES_INIT 6

typedef enum { CHEAT1, CHEAT2, CHEAT3 } cheat_t;

typedef struct
{
    uint32_t score;
    uint8_t name[12];          /* 10 + 2 for padding */
} hscore_t;

extern uint8_t game_lives;      /* lives counter */
extern uint8_t game_bombs;      /* bombs counter */
extern uint8_t game_bullets;    /* bullets counter */

extern uint32_t game_score;     /* score */

extern hscore_t game_hscores[8];  /* highest scores (hall of fame) */

extern uint16_t game_map;       /* current map */
extern uint16_t game_submap;    /* current submap */

extern bool game_chsm;       /* change submap request (TRUE, FALSE) */

extern void game_change_period(uint8_t);
extern void game_restore_period(void);

extern rect_t *game_rects; /* rectangles to redraw at each frame */

extern void game_run(void);
extern void game_setmusic(char *name, int8_t loop);

/* Cheats */
typedef struct {
    bool unlimited;     /* infinite lives, bombs and bullets */
    bool immortal;      /* never die */
    bool xrayvision;    /* highlight sprites */
} game_cheat_t;
extern game_cheat_t game_cheat;
extern void game_toggleCheat(cheat_t);

#ifdef ENABLE_SOUND
typedef struct {
    sound_t *GAMEOVER;
    sound_t *SBONUS2;
    sound_t *BULLET;
    sound_t *BOMBSHHT;
    sound_t *EXPLODE;
    sound_t *STICK;
    sound_t *WALK;
    sound_t *CRAWL;
    sound_t *JUMP;
    sound_t *PAD;
    sound_t *BOX;
    sound_t *BONUS;
    sound_t *SBONUS1;
    sound_t *DIE;
    sound_t *ENTITY[10];
} wav_t;
extern wav_t WAV;
#endif

#endif

/* eof */


