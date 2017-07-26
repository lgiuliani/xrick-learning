/*
 * xrick/src/scr_imain.c
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

#include <stdio.h>  /* sprintf */

#include "system.h"
#include "game.h"
#include "screens.h"
#include "pics.h"
#include "draw.h"
#include "control.h"
#include "sysvid.h"

typedef enum {
    START,
    DISPLAY_RICK_TITLE,
    WAIT_FOR_KEY_PRESSED_1,
    WAIT_FOR_KEY_RELEASED_1,
    DISPLAY_HALL_OF_FAME,
    WAIT_FOR_KEY_PRESSED_2,
    WAIT_FOR_KEY_RELEASED_2
} sequence_t;


/*
 * Main introduction
 *
 * return: RUNNING, DONE
 */
state_t
screen_introMain(void)
{
    static sequence_t seq = START;
    static uint8_t seen = 0;
    static bool first = true;
    static uint32_t tm = 0;
    uint8_t i, s[32];
    bool done = false;

    switch (seq)
    {
    case START:
        draw_tilesBank = 0;
        if (first)
            seq = DISPLAY_RICK_TITLE;
        else
            seq = DISPLAY_HALL_OF_FAME;
        game_change_period(50);
        game_rects = &draw_SCREENRECT;
#ifdef ENABLE_SOUND
        game_setmusic("sounds/tune5.wav", -1);
#endif
        break;
    case DISPLAY_RICK_TITLE:    /* display Rick Dangerous title and Core Design copyright */
        sysvid_clear();
        tm = sys_gettime();

        draw_pic(SYSVID_HEIGHT, pic_splash);

        seq = WAIT_FOR_KEY_PRESSED_1;
        break;

    case WAIT_FOR_KEY_PRESSED_1:  /* wait for key pressed or timeout */
        if (control.fire)
            seq = WAIT_FOR_KEY_RELEASED_1;
        else if (sys_gettime() - tm > SCREEN_TIMEOUT)
        {
            seen++;
            seq = DISPLAY_HALL_OF_FAME;
        }
        break;

    case WAIT_FOR_KEY_RELEASED_1:  /* wait for key released */
        if (!control.fire)
        {
            if (seen++ == 0)
                seq = DISPLAY_HALL_OF_FAME;
            else
                done = true;
        }
        break;

    case DISPLAY_HALL_OF_FAME:  /* dispay hall of fame */
        sysvid_clear();
        tm = sys_gettime();

        /* hall of fame title */
        draw_pic(0x20, pic_haf);

        /* hall of fame content */
        draw_setfb(56, 40);

        for (i = 0; i < 8; i++)
        {
            sprintf((char *)s, "%06u@@@....@@@%s",
                    game_hscores[i].score, game_hscores[i].name);
            s[26] = (uint8_t)'\377';
            s[27] = (uint8_t)'\377';
            s[28] = (uint8_t)'\376';
            draw_tilesList(s);
        }

        seq = WAIT_FOR_KEY_PRESSED_2;
        break;

    case WAIT_FOR_KEY_PRESSED_2:  /* wait for key pressed or timeout */
        if (control.fire)
            seq = WAIT_FOR_KEY_RELEASED_2;
        else if (sys_gettime() - tm > SCREEN_TIMEOUT)
        {
            seen++;
            seq = DISPLAY_RICK_TITLE;
        }
        break;

    case WAIT_FOR_KEY_RELEASED_2:  /* wait for key released */
        if (!control.fire)
        {
            if (seen++ == 0)
                seq = DISPLAY_RICK_TITLE;
            else
                done = true;
        }
        break;
    }

    if (done)    /* we're done */
    {
        sysvid_clear();
        seq = START;
        seen = 0;
        first = false;
        game_restore_period();
        return DONE;
    }

    return RUNNING;
}

/* eof */
