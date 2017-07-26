/*
 * xrick/src/e_bonus.c
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

#include "e_rick.h"
#include "maps.h"
#include "e_bonus.h"


/*
 * Entity action
 *
 * ASM 242C
 */
void
e_bonus_action(uint8_t e)
{
#define seq c1

    if (ents_entities[e].seq == 0)
    {
        if (e_rick_isColliding(e))
        {
            game_score += 500;
#ifdef ENABLE_SOUND
            syssnd_play(WAV.BONUS, 1);
#endif
            map_marks[ents_entities[e].mark].ent |= MAP_MARK_NACT;
            ents_entities[e].seq = 1;
            ents_entities[e].sprite = 0xad;
            ents_entities[e].front = true;
            ents_entities[e].position.y -= 0x08;
        }
    }

    else if (ents_entities[e].seq > 0 && ents_entities[e].seq < 10)
    {
        ents_entities[e].seq++;
        ents_entities[e].position.y -= 2;
    }

    else
    {
        ents_entities[e].n = 0;
    }
}


/* eof */


