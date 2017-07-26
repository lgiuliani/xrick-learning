/*
 * xrick/src/scr_gameover.c
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

#include "system.h"
#include "game.h"
#include "screens.h"
#include "sysvid.h"

#include "draw.h"
#include "control.h"

typedef enum {
    DISPLAY_BANNER,
    WAIT_FOR_KEY_PRESSED,
    WAIT_FOR_KEY_RELEASED
} seq_gameover_t;

/*
 * Display the game over screen
 *
 * return: RUNNING, DONE
 */
state_t
screen_gameover(void)
{
    static seq_gameover_t seq = DISPLAY_BANNER;
    static uint32_t tm = 0;
    bool done = false;

    /*
    #ifdef ENABLE_SOUND
    	static sound_t *snd;
    	static uint8_t chan;
    #endif
    */

    switch (seq)
    {
    case DISPLAY_BANNER: /* display banner */
        draw_tilesBank = 0;
        game_change_period(50);
#ifdef ENABLE_SOUND
        game_setmusic("sounds/gameover.wav", 1);
#endif
        sysvid_clear();
        tm = sys_gettime();
        draw_setfb(120, 80);
        draw_tilesList(screen_gameovertxt);

        game_rects = &draw_SCREENRECT;
        seq = WAIT_FOR_KEY_PRESSED;
        break;

    case WAIT_FOR_KEY_PRESSED:  /* wait for key pressed */
        if (control.fire)
            seq = WAIT_FOR_KEY_RELEASED;
        else if (sys_gettime() - tm > SCREEN_TIMEOUT)
            done = true;
        else
            sys_sleep(50);
        break;

    case WAIT_FOR_KEY_RELEASED:  /* wait for key released */
        if (!control.fire)
            done = true;
        else
            sys_sleep(50);
        break;
    }

    /* we're done */
    if (done)
    {
        sysvid_clear();
        seq = DISPLAY_BANNER;
        game_restore_period();
        return DONE;
    }

    return RUNNING;
}

/* eof */

