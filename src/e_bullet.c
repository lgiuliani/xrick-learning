/*
 * xrick/src/e_bullet.c
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
#include "util.h"

/*
 * public vars (for performance reasons)
 */
static int8_t e_bullet_offsx;
static pos_t e_bullet_c;


/*
 * Bullet hit test
 *
 * ASM 11CD
 * returns: TRUE/hit, FALSE/not
 */
bool e_bullet_isHitting(uint8_t e)
{
    if (E_BULLET_ENT.n && u_isEntityCollidePoint(e, e_bullet_c))
    {
        E_BULLET_ENT.n = 0;
        return true;
    }

    return false;
}

/*
 * Tigger box test
 *
 *
 *
 */
bool
e_bullet_isTriggering(uint8_t e)
{
    if (E_BULLET_ENT.n && u_trigbox(e, e_bullet_c))
    {
        E_BULLET_ENT.n = 0;
        return true;
    }

    return false;
}

/*
 * Initialize bullet
 */
void
e_bullet_init(pos_t position, dir_t dir)
{
    E_BULLET_ENT.n = 0x02;
    E_BULLET_ENT.position = position;
    E_BULLET_ENT.position.y += 0x0006;
    E_BULLET_ENT.dimension = (dim_t){0x18, 0x10};
    if (dir == RIGHT)
    {
        e_bullet_offsx = 0x08;
        E_BULLET_ENT.sprite = 0x20;
    }
    else
    {
        e_bullet_offsx = -0x08;
        E_BULLET_ENT.sprite = 0x21;
    }
#ifdef ENABLE_SOUND
    syssnd_play(WAV.BULLET, 1);
#endif
}


/*
 * Entity action
 *
 * ASM 1883, 0F97
 */
void
e_bullet_action(UNUSED(uint8_t e))
{
    /* move bullet */
    E_BULLET_ENT.position.x += e_bullet_offsx;

    if (E_BULLET_ENT.position.x <= -0x10 || E_BULLET_ENT.position.x > 0xe8)
    {
        /* out: deactivate */
        E_BULLET_ENT.n = 0;
    }
    else
    {
        /* update bullet center coordinates */
        e_bullet_c.x = E_BULLET_ENT.position.x + 0x0c;
        e_bullet_c.y = E_BULLET_ENT.position.y + 0x05;
        if (map_eflg[map_map[e_bullet_c.y >> 3][e_bullet_c.y >> 3]].solid)
        {
            /* hit something: deactivate */
            E_BULLET_ENT.n = 0;
        }
    }
}



/* eof */


