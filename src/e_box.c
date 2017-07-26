/*
 * xrick/src/e_box.c
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
#include "ents.h"

#include "e_bullet.h"
#include "e_bomb.h"
#include "maps.h"
#include "e_box.h"

/*
 * FIXME this is because the same structure is used
 * for all entities. Need to replace this w/ an inheritance
 * solution.
 */
#define cnt c1

/*
 * Constants
 */
#define SEQ_INIT 0x0A

/*
 * Prototypes
 */
static void explode(uint8_t);

/*
 * Entity action
 *
 * ASM 245A
 */
void
e_box_action(uint8_t e)
{
    static uint8_t sp[] = {0x24, 0x25, 0x26, 0x27, 0x28};  /* explosion sprites sequence */

    if (ents_entities[e].n & ENT_LETHAL)
    {
        /*
         * box is lethal i.e. exploding
         * play sprites sequence then stop
         */
        ents_entities[e].sprite = sp[ents_entities[e].cnt >> 1];
        if (--ents_entities[e].cnt == 0)
        {
            ents_entities[e].n = 0;
            map_marks[ents_entities[e].mark].ent |= MAP_MARK_NACT;
        }
    }
    else
    {
        /*
         * not lethal: check to see if triggered
         */
        if (e_rick_isColliding(e))
        {
            /* rick: collect bombs or bullets and stop */
#ifdef ENABLE_SOUND
            syssnd_play(WAV.BOX, 1);
#endif
            if (ents_entities[e].n == 0x10)
                game_bombs = GAME_BOMBS_INIT;
            else  /* 0x11 */
                game_bullets = GAME_BULLETS_INIT;
            ents_entities[e].n = 0;
            map_marks[ents_entities[e].mark].ent |= MAP_MARK_NACT;
        }
        /* rick's stick, bullet, bomb: explode */
        else if (e_rick_isBlocking(e) ||
                 e_bullet_isHitting(e) ||
                 e_bomb_isExplosing(e))
        {
            explode(e);
        }
    }
}


/*
 * Explode when
 */
static void explode(uint8_t e)
{
    ents_entities[e].cnt = SEQ_INIT;
    ents_entities[e].n |= ENT_LETHAL;
#ifdef ENABLE_SOUND
    syssnd_play(WAV.EXPLODE, 1);
#endif
}

/* eof */


