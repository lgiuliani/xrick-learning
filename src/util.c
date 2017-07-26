/*
 * xrick/src/util.c
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

#include <stdlib.h>  /* NULL */

#include "system.h"
#include "game.h"
#include "util.h"

#include "e_rick.h"

/*
 * Full box test.
 *
 * ASM 1199
 *
 * e: entity to test against.
 * x,y: coordinates to test.
 * ret: TRUE/(x,y) is within e's space, FALSE/not.
 */
bool
u_isEntityCollidePoint(uint8_t e, pos_t point)
{
    ent_t entity = ents_entities[e];

    if (entity.position.x >= point.x ||
            entity.position.x + entity.dimension.width < point.x ||
            entity.position.y >= point.y ||
            entity.position.y + entity.dimension.height < point.y)
        return false;

    return true;
}


/*
 * Box test (then whole e2 is checked agains the center of e1).
 *
 * ASM 113E
 *
 * e1: entity to test against (corresponds to DI in asm code).
 * e2: entity to test (corresponds to SI in asm code).
 * ret: TRUE/intersect, FALSE/not.
 */
bool
u_isEntitiesColliding(uint8_t e1, uint8_t e2)
{
    ent_t entity1 = ents_entities[e1];
    ent_t entity2 = ents_entities[e2];

    /* rick is special (may be crawling) */
    if (e1 == E_RICK_NO)
        return e_rick_isColliding(e2);

    /*
     * entity 1: x+0x05 to x+0x011, y to y+0x14
     * entity 2: x to x+ .w, y to y+ .h
     */
    if (entity1.position.x + 0x11 < entity2.position.x ||
            entity1.position.x + 0x05 > entity2.position.x + entity2.dimension.width ||
            entity1.position.y + 0x14 < entity2.position.y ||
            entity1.position.y > entity2.position.y + entity2.dimension.height - 1)
        return false;

    return true;
}


/*
 * Compute the environment flag.
 *
 * ASM 0FBC if !crawl, else 103E
 *
 * x, y: coordinates where to compute the environment flag
 * crawl: is rick crawling?
 * rc0: anything CHANGED to the environment flag for crawling (6DBA)
 * rc1: anything CHANGED to the environment flag (6DAD)
 */
map_flags_t
u_envtest(int16_t x, int16_t y, bool crawl)
{
    uint_fast8_t i,j;
    uint8_t xx;
    map_flags_t env1 = {false, false, false, false, false, false, false, false};

    /* prepare for ent #0 test */
    ents_entities[ENT_ENTSNUM].position.x = x;
    ents_entities[ENT_ENTSNUM].position.y = y;

    i = crawl ? 0 : 1;
    if (y & 0x0004) i++;

    x += 4;
    xx = (uint8_t)x; /* FIXME? */

    x = x >> 3;  /* from pixels to tiles */
    y = y >> 3;  /* from pixels to tiles */

    if (crawl)
        y++;

    if (xx & 0x07)    /* tiles columns alignment */
    {
        do
        {
            for (j=0; j < 3; j++)
            {
                env1.solid |= map_eflg[map_map[y][x+j]].solid;
                env1.superpad |= map_eflg[map_map[y][x+j]].superpad;
                env1.foreground |= map_eflg[map_map[y][x+j]].foreground;
                env1.lethal |= map_eflg[map_map[y][x+j]].lethal;
                env1.once |= map_eflg[map_map[y][x+j]].once;
            }
            env1.climb |= map_eflg[map_map[y][x + 1]].climb;

            y++;
        }
        while (i--);

        env1.solid |= map_eflg[map_map[y][x]].solid;
        env1.superpad |= map_eflg[map_map[y][x]].superpad;
        env1.wayup |= map_eflg[map_map[y][x]].wayup;
        env1.foreground |= map_eflg[map_map[y][x]].foreground;
        env1.lethal |= map_eflg[map_map[y][x]].lethal;
        env1.once |= map_eflg[map_map[y][x]].once;

        //env1 |= map_eflg[map_map[y][x + 1]];
        env1.vertical |= map_eflg[map_map[y][x+1]].vertical;
        env1.solid |= map_eflg[map_map[y][x+1]].solid;
        env1.superpad |= map_eflg[map_map[y][x+1]].superpad;
        env1.wayup |= map_eflg[map_map[y][x+1]].wayup;
        env1.foreground |= map_eflg[map_map[y][x+1]].foreground;
        env1.lethal |= map_eflg[map_map[y][x+1]].lethal;
        env1.climb |= map_eflg[map_map[y][x+1]].climb;
        env1.once  |= map_eflg[map_map[y][x+1]].once;

        env1.solid |= map_eflg[map_map[y][x+2]].solid;
        env1.superpad |= map_eflg[map_map[y][x+2]].superpad;
        env1.wayup |= map_eflg[map_map[y][x+2]].wayup;
        env1.foreground |= map_eflg[map_map[y][x+2]].foreground;
        env1.lethal |= map_eflg[map_map[y][x+2]].lethal;
        env1.once |= map_eflg[map_map[y][x+2]].once;
    }
    else
    {
        do
        {
            env1.solid |= map_eflg[map_map[y][x]].solid;
            env1.superpad |= map_eflg[map_map[y][x]].superpad;
            env1.foreground |= map_eflg[map_map[y][x]].foreground;
            env1.lethal |= map_eflg[map_map[y][x]].lethal;
            env1.climb |= map_eflg[map_map[y][x]].climb;
            env1.once  |= map_eflg[map_map[y][x]].once;

            env1.solid |= map_eflg[map_map[y][x+1]].solid;
            env1.superpad |= map_eflg[map_map[y][x+1]].superpad;
            env1.foreground |= map_eflg[map_map[y][x+1]].foreground;
            env1.lethal |= map_eflg[map_map[y][x+1]].lethal;
            env1.climb |= map_eflg[map_map[y][x+1]].climb;
            env1.once  |= map_eflg[map_map[y][x+1]].once;

            y++;
        }
        while (i--);

        env1.vertical |= map_eflg[map_map[y][x]].vertical;
        env1.solid |= map_eflg[map_map[y][x]].solid;
        env1.superpad |= map_eflg[map_map[y][x]].superpad;
        env1.wayup |= map_eflg[map_map[y][x]].wayup;
        env1.foreground |= map_eflg[map_map[y][x]].foreground;
        env1.lethal |= map_eflg[map_map[y][x]].lethal;
        env1.climb |= map_eflg[map_map[y][x]].climb;
        env1.once  |= map_eflg[map_map[y][x]].once;

        env1.vertical |= map_eflg[map_map[y][x+1]].vertical;
        env1.solid |= map_eflg[map_map[y][x+1]].solid;
        env1.superpad |= map_eflg[map_map[y][x+1]].superpad;
        env1.wayup |= map_eflg[map_map[y][x+1]].wayup;
        env1.foreground |= map_eflg[map_map[y][x+1]].foreground;
        env1.lethal |= map_eflg[map_map[y][x+1]].lethal;
        env1.climb |= map_eflg[map_map[y][x+1]].climb;
        env1.once  |= map_eflg[map_map[y][x+1]].once;

    }

    /*
     * If not lethal yet, and there's an entity on slot zero, and (x,y)
     * boxtests this entity, then raise SOLID flag. This is how we make
     * sure that no entity can move over the entity that is on slot zero.
     *
     * Beware! When game_cheat2 is set, this means that a block can
     * move over rick without killing him -- but then rick is trapped
     * because the block is solid.
     */
    if (!(env1.lethal)
            && ents_entities[0].n
            && u_isEntitiesColliding(ENT_ENTSNUM, 0))
    {
        env1.solid = true;
    }

    /* When game_cheat2 is set, the environment can not be lethal. */
    if (game_cheat.immortal)
        env1.lethal = false;

    return env1;
}

bool u_envtestcrawl(int16_t x, int16_t y)
{
    uint8_t xx;
    map_flags_t env0 = {false, false, false, false, false, false, false, false};

    x += 4;
    xx = (uint8_t)x; /* FIXME? */

    x = x >> 3;  /* from pixels to tiles */
    y = y >> 3;  /* from pixels to tiles */

    env0.vertical |= map_eflg[map_map[y][x]].vertical;
    env0.solid |= map_eflg[map_map[y][x]].solid;
    env0.superpad |= map_eflg[map_map[y][x]].superpad;
    env0.wayup |= map_eflg[map_map[y][x]].wayup;

    env0.vertical |= map_eflg[map_map[y][x+1]].vertical;
    env0.solid |= map_eflg[map_map[y][x+1]].solid;
    env0.superpad |= map_eflg[map_map[y][x+1]].superpad;
    env0.wayup |= map_eflg[map_map[y][x+1]].wayup;

    if (xx & 0x07)    /* tiles columns alignment */
    {
        env0.vertical |= map_eflg[map_map[y][x+2]].vertical;
        env0.solid |= map_eflg[map_map[y][x+2]].solid;
        env0.superpad |= map_eflg[map_map[y][x+2]].superpad;
        env0.wayup |= map_eflg[map_map[y][x+2]].wayup;
    }

    // env0.climb || env0.foreground || env0.lethal || env0.once not used
    if (env0.solid || env0.superpad || env0.vertical || env0.wayup)
            return true;

    return false;
}


/*
 * Check if x,y is within e trigger box.
 *
 * ASM 126F
 * return: FALSE if not in box, TRUE if in box.
 */
bool
u_trigbox(uint8_t e, pos_t point)
{
    uint16_t xmax, ymax;

    xmax = (uint16_t) (ents_entities[e].trigger.x + (ent_entdata[ents_entities[e].n & 0x7F].trig_w << 3));
    ymax = (uint16_t) (ents_entities[e].trigger.y + (ent_entdata[ents_entities[e].n & 0x7F].trig_h << 3));

    if (xmax > 0xFF)
        xmax = 0xFF;

    if (point.x <= ents_entities[e].trigger.x || point.x > xmax ||
        point.y <= ents_entities[e].trigger.y || point.y > ymax)
        return false;

    return true;
}


void u_calc_new_y(uint8_t e, int16_t *new_y, uint8_t *new_ylow)
{
    uint32_t i;

    /* calc new y */
    i = (uint32_t) ((ents_entities[e].position.y << 8) + ents_entities[e].offsy + ents_entities[e].ylow);
    //i = (uint32_t) (((E_RICK_ENT.position.y << 8) | E_RICK_ENT.ylow) + E_RICK_ENT.offsy);
    *new_y = (int16_t) (i >> 8);
    *new_ylow = (uint8_t) i;
}

/* eof */
