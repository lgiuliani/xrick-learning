/*
 * xrick/src/e_sbonus.c
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
#include "e_sbonus.h"

#include "maps.h"
#include "e_rick.h"


/*
 * public vars
 */
bool e_sbonus_counting = false;

/*
 * Pivate vars
 */
static uint8_t e_sbonus_counter = 0;
static uint16_t e_sbonus_bonus = 0;


/*
 * Entity action / start counting
 *
 * ASM 2182
 */
void
e_sbonus_start(uint8_t e)
{
    ents_entities[e].sprite = 0; /* invisible */
    if (e_rick_isTriggering(e))
    {
        /* rick is within trigger box */
        ents_entities[e].n = 0;
        e_sbonus_counting = true;  /* 6DD5 */
        e_sbonus_counter = 0x1e;  /* 6DDB */
        e_sbonus_bonus = 2000;    /* 291A-291D */
#ifdef ENABLE_SOUND
        syssnd_play(WAV.SBONUS1, 1);
#endif
    }
}


/*
 * Entity action / stop counting
 *
 * ASM 2143
 */
void
e_sbonus_stop(uint8_t e)
{
    ents_entities[e].sprite = 0; /* invisible */

    if (!e_sbonus_counting)
        return;

    if (e_rick_isTriggering(e))
    {
        /* rick is within trigger box */
        e_sbonus_counting = false;  /* stop counting */
        ents_entities[e].n = 0;  /* deactivate entity */
        game_score += e_sbonus_bonus;  /* add bonus to score */
#ifdef ENABLE_SOUND
        syssnd_play(WAV.SBONUS2, 1);
#endif
        /* make sure the entity won't be activated again */
        map_marks[ents_entities[e].mark].ent |= MAP_MARK_NACT;
    }
    else
    {
        /* keep counting */
        if (--e_sbonus_counter == 0)
        {
            e_sbonus_counter = 0x1e;
            if (e_sbonus_bonus) e_sbonus_bonus--;
        }
    }
}

/* eof */


