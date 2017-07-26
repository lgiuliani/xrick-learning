/*
 * xrick/src/scr_getname.c
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
    PREPARE_SCREEN,
    WAIT_FOR_KEY_PRESSED,
    WAIT_FOR_FIRE_RELEASED,
    WAIT_FOR_UP_RELEASED,
    WAIT_FOR_DOWN_RELEASED,
    WAIT_FOR_LEFT_RELEASED,
    WAIT_FOR_RIGHT_RELEASED
} seq_getname_t;


/*
 * local vars
 */
static seq_getname_t seq = PREPARE_SCREEN;
static uint8_t x, y, p;
static uint8_t name[10];

#define TILE_POINTER '\072'
#define TILE_CURSOR '\073'
#define TOPLEFT_X 116
#define TOPLEFT_Y 64
#define NAMEPOS_X 120
#define NAMEPOS_Y 160
#define AUTOREPEAT_TMOUT 100


/*
 * prototypes
 */
static void pointer_show(bool);
static void name_update(void);
static void name_draw(void);


/*
 * Get name
 *
 * return: 0 while running, 1 when finished.
 */
state_t
screen_getname(void)
{
    static uint32_t tm = 0;
    uint8_t i, j;
    bool done = false;

    switch (seq)
    {
    case PREPARE_SCREEN:  /* prepare screen */
        /* figure out if this is a high score */
        if (game_score < game_hscores[7].score)
            return DONE;

        /* prepare */
        draw_tilesBank = 0;
        for (i = 0; i < 10; i++)
            name[i] = '@';
        x = y = p = 0;
        game_rects = &draw_SCREENRECT;

        sysvid_clear();
        draw_setfb(76, 40);
        draw_tilesList((uint8_t *)"PLEASE@ENTER@YOUR@NAME\376");
        for (i = 0; i < 6; i++)
            for (j = 0; j < 4; j++)
            {
                draw_setfb(TOPLEFT_X + i * 8 * 2, TOPLEFT_Y + j * 8 * 2);
                draw_tile('A' + i + j * 6);
            }
        draw_setfb(TOPLEFT_X, TOPLEFT_Y + 64);
        draw_tilesList((uint8_t *)"Y@Z@.@@@\074\373\374\375\376");

        name_draw();
        pointer_show(true);
        seq = WAIT_FOR_KEY_PRESSED;
        break;

    case WAIT_FOR_KEY_PRESSED:  /* wait for key pressed */
        if (control.fire)
            seq = WAIT_FOR_FIRE_RELEASED;
        if (control.up)
        {
            if (y > 0)
            {
                pointer_show(false);
                y--;
                pointer_show(true);
                tm = sys_gettime();
            }
            seq = WAIT_FOR_UP_RELEASED;
        }
        if (control.down)
        {
            if (y < 4)
            {
                pointer_show(false);
                y++;
                pointer_show(true);
                tm = sys_gettime();
            }
            seq = WAIT_FOR_DOWN_RELEASED;
        }
        if (control.left)
        {
            if (x > 0)
            {
                pointer_show(false);
                x--;
                pointer_show(true);
                tm = sys_gettime();
            }
            seq = WAIT_FOR_LEFT_RELEASED;
        }
        if (control.right)
        {
            if (x < 5)
            {
                pointer_show(false);
                x++;
                pointer_show(true);
                tm = sys_gettime();
            }
            seq = WAIT_FOR_RIGHT_RELEASED;
        }
        if (seq == WAIT_FOR_KEY_PRESSED)
            sys_sleep(50);
        break;

    case WAIT_FOR_FIRE_RELEASED:  /* wait for FIRE released */
        if (!control.fire)
        {
            if (x == 5 && y == 4)    /* end */
            {
                i = 0;
                while (game_score < game_hscores[i].score)
                    i++;
                j = 7;
                while (j > i)
                {
                    game_hscores[j].score = game_hscores[j - 1].score;
                    for (x = 0; x < 10; x++)
                        game_hscores[j].name[x] = game_hscores[j - 1].name[x];
                    j--;
                }
                game_hscores[i].score = game_score;
                for (x = 0; x < 10; x++)
                    game_hscores[i].name[x] = name[x];
                done = true;
            }
            else
            {
                name_update();
                name_draw();
                seq = WAIT_FOR_KEY_PRESSED;
            }
        }
        else
            sys_sleep(50);
        break;

    case WAIT_FOR_UP_RELEASED:  /* wait for UP released */
        if (!control.up || (sys_gettime() - tm > AUTOREPEAT_TMOUT))
            seq = WAIT_FOR_KEY_PRESSED;
        else
            sys_sleep(50);
        break;

    case WAIT_FOR_DOWN_RELEASED:  /* wait for DOWN released */
        if (!control.down || (sys_gettime() - tm > AUTOREPEAT_TMOUT))
            seq = WAIT_FOR_KEY_PRESSED;
        else
            sys_sleep(50);
        break;

    case WAIT_FOR_LEFT_RELEASED:  /* wait for LEFT released */
        if (!control.left || (sys_gettime() - tm > AUTOREPEAT_TMOUT))
            seq = WAIT_FOR_KEY_PRESSED;
        else
            sys_sleep(50);
        break;

    case WAIT_FOR_RIGHT_RELEASED:  /* wait for RIGHT released */
        if (!control.right || (sys_gettime() - tm > AUTOREPEAT_TMOUT))
            seq = WAIT_FOR_KEY_PRESSED;
        else
            sys_sleep(50);
        break;
    }

    if (done)    /* seq 99, we're done */
    {
        sysvid_clear();
        seq = PREPARE_SCREEN;
        return DONE;
    }

    return RUNNING;
}


static void
pointer_show(bool show)
{
    draw_setfb(TOPLEFT_X + x * 8 * 2, TOPLEFT_Y + y * 8 * 2 + 8);

    draw_tile((show)?TILE_POINTER:'@');
}

static void
name_update(void)
{
    uint8_t i;

    i = x + y * 6;
    if (i < 26 && p < 10)
        name[p++] = 'A' + i;
    if (i == 26 && p < 10)
        name[p++] = '.';
    if (i == 27 && p < 10)
        name[p++] = '@';
    if (i == 28 && p > 0)
    {
        p--;
    }
}

static void
name_draw(void)
{
    uint8_t i;

    draw_setfb(NAMEPOS_X, NAMEPOS_Y);

    for (i = 0; i < p; i++)
        draw_tile(name[i]);
    for (i = p; i < 10; i++)
        draw_tile(TILE_CURSOR);

    draw_setfb(NAMEPOS_X, NAMEPOS_Y + 8);
    for (i = 0; i < 10; i++)
        draw_tile('@');
    draw_setfb(NAMEPOS_X + 8 * (p < 9 ? p : 9), NAMEPOS_Y + 8);
    draw_tile(TILE_POINTER);
}


/* eof */
