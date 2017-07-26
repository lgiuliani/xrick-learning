/*
 * xrick/src/scroller.c
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

#include <stdlib.h>

#include "system.h"
#include "game.h"
#include "scroller.h"
#include "debug.h"

#include "draw.h"
#include "maps.h"


static const uint8_t SCROLL_PERIOD=24;

static void display(void);

/*
 * Scroll up
 *
 */
state_t
scroll_up(void)
{
    uint8_t i, j;
    static uint8_t n = 0;

    /* last call: restore */
    if (n == 8)
    {
        n = 0;
        game_restore_period();
        return DONE;
    }

    /* first call: prepare */
    if (n == 0)
        game_change_period(SCROLL_PERIOD);

    /* translate map */
    for (i = MAP_ROW_SCRTOP; i < MAP_ROW_HBBOT; i++)
        for (j = 0x00; j < 0x20; j++)
            map_map[i][j] = map_map[i + 1][j];

    /* translate entities */
    for (i = 0; ents_entities[i].n != 0xFF; i++)
    {
        if (ents_entities[i].n)
        {
            ents_entities[i].saved.y -= 8;
            ents_entities[i].trigger.y -= 8;
            ents_entities[i].position.y -= 8;
            if (ents_entities[i].position.y & 0x8000)    /* map coord. from 0x0000 to 0x0140 */
            {
                IFDEBUG_SCROLLER(
                    sys_printf("xrick/scroller: entity %#04X is gone\n", i);
                );
                ents_entities[i].n = 0;
            }
        }
    }

    /* display */
    map_frow++;

    /* loop */
    if (n++ == 7)
    {
        /* activate visible entities */
        ent_actvis((uint8_t) (map_frow + MAP_ROW_HBTOP), (uint8_t) (map_frow + MAP_ROW_HBBOT));

        /* prepare map */
        map_expand();
    }

    display();
    return RUNNING;
}

/*
 * Scroll down
 *
 */
state_t
scroll_down(void)
{
    uint8_t i, j;
    static uint8_t n = 0;

    /* last call: restore */
    if (n == 8)
    {
        n = 0;
        game_restore_period();
        return DONE;
    }

    /* first call: prepare */
    if (n == 0)
        game_change_period(SCROLL_PERIOD);

    /* translate map */
    for (i = MAP_ROW_SCRBOT; i > MAP_ROW_HTTOP; i--)
        for (j = 0x00; j < 0x20; j++)
            map_map[i][j] = map_map[i - 1][j];

    /* translate entities */
    for (i = 0; ents_entities[i].n != 0xFF; i++)
    {
        if (ents_entities[i].n)
        {
            ents_entities[i].saved.y += 8;
            ents_entities[i].trigger.y += 8;
            ents_entities[i].position.y += 8;
            if (ents_entities[i].position.y > SYSVID_WIDTH)    /* map coord. from 0x0000 to 0x0140 */
                ents_entities[i].n = 0;
        }
    }

    /* display */
    map_frow--;

    /* loop */
    if (n++ == 7)
    {
        /* activate visible entities */
        ent_actvis((uint8_t) (map_frow + MAP_ROW_HTTOP), (uint8_t) (map_frow + MAP_ROW_HTBOT));

        /* prepare map */
        map_expand();
    }

    display();
    return RUNNING;
}

static void
display(void)
{
    draw_map();
    ent_draw();
    draw_drawStatus();

    game_rects = &draw_SCREENRECT;
}

/* eof */
