/*
 * xrick/src/e_bomb.c
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

#include "game.h"
#include "ents.h"
#include "e_bomb.h"
#include "util.h"

#include "e_rick.h"

/*
 * private vars
 */
static const uint8_t E_BOMB_TICKER = 0x2D;
static uint8_t e_bomb_ticker;
static pos_t e_bomb_c;
static bool e_bomb_lethal;

/*
 * Bomb hit test
 *
 * ASM 11CD
 * returns: TRUE/hit, FALSE/not
 */
bool e_bomb_isExplosing(uint8_t e)
{
    if (!e_bomb_lethal)
        return false;
    if (ents_entities[e].position.x > (E_BOMB_ENT.position.x >= 0xE0 ? 0xFF : E_BOMB_ENT.position.x + 0x20))
        return false;
    if (ents_entities[e].position.x + ents_entities[e].dimension.width < (E_BOMB_ENT.position.x > 0x04 ? E_BOMB_ENT.position.x - 0x04 : 0))
        return false;
    if (ents_entities[e].position.y > (E_BOMB_ENT.position.y + 0x1D))
        return false;
    if (ents_entities[e].position.y + ents_entities[e].dimension.height < (E_BOMB_ENT.position.y > 0x04 ? E_BOMB_ENT.position.y - 0x04 : 0))
        return false;
    return true;
}

/*
 * Initialize bomb
 */
void e_bomb_init(pos_t position)
{
    /*
     * Atari ST dynamite sprites are not centered the
     * way IBM PC sprites were ... need to adjust things a little bit
     */
    E_BOMB_ENT.n = 0x03;
    E_BOMB_ENT.position = position;
    E_BOMB_ENT.position.x += 4;
    E_BOMB_ENT.position.y += 5;
    E_BOMB_ENT.dimension = (dim_t){0x20, 0x15};
    e_bomb_ticker = E_BOMB_TICKER;
    e_bomb_lethal = false;
}


/*
 * Entity action
 *
 * ASM 18CA
 */
void
e_bomb_action(UNUSED(uint8_t e))
{
    /* tick */
    e_bomb_ticker--;

    if (e_bomb_ticker == 0)
    {
        /*
         * end: deactivate
         */
        E_BOMB_ENT.n = 0;
        e_bomb_lethal = false;
    }
    else if (e_bomb_ticker >= 0x0A)
    {
        /*
         * ticking
         */
#ifdef ENABLE_SOUND
        if ((e_bomb_ticker & 0x03) == 0x02)
            syssnd_play(WAV.BOMBSHHT, 1);
#endif
        /* ST bomb sprites sequence is longer */
        if (e_bomb_ticker < 40)
            E_BOMB_ENT.sprite = 0x99 + 19 - (e_bomb_ticker >> 1);
        else
            E_BOMB_ENT.sprite = (e_bomb_ticker & 0x01) ? 0x23 : 0x22;
    }
    else if (e_bomb_ticker == 0x09)
    {
        /*
         * explode
         */
#ifdef ENABLE_SOUND
        syssnd_play(WAV.EXPLODE, 1);
#endif
        /* See above: fixing alignment */
        E_BOMB_ENT.sprite = 0xa8 + 4 - (e_bomb_ticker >> 1);

        e_bomb_c.x = E_BOMB_ENT.position.x + 0x0C - 4;
        e_bomb_c.y = E_BOMB_ENT.position.y + 0x000A - 5;
        e_bomb_lethal = true;
        if (e_bomb_isExplosing(E_RICK_NO))
            e_rick_gozombie();
    }
    else
    {
        /*
         * exploding
         */
        E_BOMB_ENT.sprite = 0xa8 + 4 - (e_bomb_ticker >> 1);

        /* exploding, hence lethal */
        if (e_bomb_isExplosing(E_RICK_NO))
            e_rick_gozombie();
    }
}

bool
e_bomb_isTriggering(uint8_t e)
{
    return (e_bomb_lethal && u_trigbox(e, e_bomb_c));
}


/* eof */
