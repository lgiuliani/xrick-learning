/*
 * xrick/src/ents.c
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
#include "ents.h"
#include "e_bullet.h"
#include "e_bomb.h"
#include "e_them.h"
#include "e_bonus.h"
#include "e_box.h"
#include "e_sbonus.h"
#include "maps.h"
#include "draw.h"

/*
 * global vars
 */
const uint8_t ENT_LETHAL = 0x80;
ent_t ents_entities[ENT_ENTSNUM + 1];
rect_t *ent_rects = NULL;

/* Table containing entity action function pointers. */
static void (*ent_actf[])(uint8_t) =
        {
                NULL,        /* 00 - zero means that the slot is free */
                e_rick_action,   /* 01 - 12CA */
                e_bullet_action,  /* 02 - 1883 */
                e_bomb_action,  /* 03 - 18CA */
                e_them_t1a_action,  /* 04 - 2452 */
                e_them_t1b_action,  /* 05 - 21CA */
                e_them_t2_action,  /* 06 - 2718 */
                e_them_t1a_action,  /* 07 - 2452 */
                e_them_t1b_action,  /* 08 - 21CA */
                e_them_t2_action,  /* 09 - 2718 */
                e_them_t1a_action,  /* 0A - 2452 */
                e_them_t1b_action,  /* 0B - 21CA */
                e_them_t2_action,  /* 0C - 2718 */
                e_them_t1a_action,  /* 0D - 2452 */
                e_them_t1b_action,  /* 0E - 21CA */
                e_them_t2_action,  /* 0F - 2718 */
                e_box_action,  /* 10 - 245A */
                e_box_action,  /* 11 - 245A */
                e_bonus_action,  /* 12 - 242C */
                e_bonus_action,  /* 13 - 242C */
                e_bonus_action,  /* 14 - 242C */
                e_bonus_action,  /* 15 - 242C */
                e_sbonus_start,  /* 16 - 2182 */
                e_sbonus_stop  /* 17 - 2143 */
        };

/*
 * prototypes
 */
static void ent_backgroundLoop(bool);
static void ent_foregroundLoop(void);
static void ent_rectanglesLoop(bool);
static void ent_addrect(pos_t, dim_t);
static bool ent_creat1(uint8_t *);
static bool ent_creat2(uint8_t *, uint16_t);
static inline ents_flags_t get_map_flags(uint16_t);


/*
 * Reset entities
 *
 * ASM 2520
 */
void ent_reset(void)
{
    uint8_t e;

    ents_entities[0].n = 0;
    for (e = 2; ents_entities[e].n != 0xff; e++)
        ents_entities[e].n = 0;
}


/*
 * Create an entity on slots 4 to 8 by using the first slot available.
 * Entities of type e_them on slots 4 to 8, when lethal, can kill
 * other e_them (on slots 4 to C) as well as rick.
 *
 * ASM 209C
 *
 * e: anything, CHANGED to the allocated entity number.
 * return: TRUE/OK FALSE/not
 */
bool ent_creat1(uint8_t *e)
{
    /* look for a slot */
    for (*e = 0x04; *e < 0x09; (*e)++)
        if (ents_entities[*e].n == 0)    /* if slot available, use it */
        {
            ents_entities[*e].c1 = 0;
            return true;
        }

    return false;
}


/*
 * Create an entity on slots 9 to C by using the first slot available.
 * Entities of type e_them on slots 9 to C can kill rick when lethal,
 * but they can never kill other e_them.
 *
 * ASM 20BC
 *
 * e: anything, CHANGED to the allocated entity number.
 * m: number of the mark triggering the creation of the entity.
 * ret: TRUE/OK FALSE/not
 */
bool ent_creat2(uint8_t *e, uint16_t m)
{
    /* make sure the entity created by this mark is not active already */
    for (*e = 0x09; *e < 0x0c; (*e)++)
        if (ents_entities[*e].n != 0 && ents_entities[*e].mark == m)
            return false;

    /* look for a slot */
    for (*e = 0x09; *e < 0x0c; (*e)++)
        if (ents_entities[*e].n == 0)    /* if slot available, use it */
        {
            ents_entities[*e].c1 = 2;
            return true;
        }

    return false;
}


/*
 * Process marks that are within the visible portion of the map,
 * and create the corresponding entities.
 *
 * absolute map coordinate means that they are not relative to
 * map_frow, as any other coordinates are.
 *
 * ASM 1F40
 *
 * frow: first visible row of the map -- absolute map coordinate
 * lrow: last visible row of the map -- absolute map coordinate
 */
void ent_actvis(uint8_t frow, uint8_t lrow)
{
    uint16_t m;
    uint8_t e;
    uint16_t y;

    /*
    * go through the list and find the first mark that
    * is visible, i.e. which has a row greater than the
    * first row (marks being ordered by row number).
    */
    for (m = map_submaps[game_submap].mark;
         map_marks[m].row != 0xff && map_marks[m].row < frow;
         m++);

    if (map_marks[m].row == 0xff)  /* none found */
        return;

    /*
    * go through the list and process all marks that are
    * visible, i.e. which have a row lower than the last
    * row (marks still being ordered by row number).
    */
    for (; map_marks[m].row != 0xff && map_marks[m].row < lrow; m++) {

        /* ignore marks that are not active */
        if (map_marks[m].ent & MAP_MARK_NACT)
            continue;

        /*
         * allocate a slot to the new entity
         *
         * slot type
         *  0   available for e_them (lethal to other e_them, and stops entities
         *      i.e. entities can't move over them. E.g. moving blocks. But they
         *      can move over entities and kill them!).
         *  1   xrick
         *  2   bullet
         *  3   bomb
         * 4-8  available for e_them, e_box, e_bonus or e_sbonus (lethal to
         *      other e_them, identified by their number being >= 0x10)
         * 9-C  available for e_them, e_box, e_bonus or e_sbonus (not lethal to
         *      other e_them, identified by their number being < 0x10)
         *
         * the type of an entity is determined by its .n as detailed below.
         *
         * 1               xrick
         * 2               bullet
         * 3               bomb
         * 4, 7, a, d      e_them, type 1a
         * 5, 8, b, e      e_them, type 1b
         * 6, 9, c, f      e_them, type 2
         * 10, 11          box
         * 12, 13, 14, 15  bonus
         * 16, 17          speed bonus
         * >17             e_them, type 3
         * 47              zombie
         */

        if (!(get_map_flags(m).stoprick)) {
            if (map_marks[m].ent >= 0x10) {
                /* boxes, bonuses and type 3 e_them go to slot 4-8 */
                /* (c1 set to 0 -> all type 3 e_them are sleeping) */
                if (!ent_creat1(&e)) continue;
            } else {
                /* type 1 and 2 e_them go to slot 9-c */
                /* (c1 set to 2) */
                if (!ent_creat2(&e, m)) continue;
            }
        } else {
            /* entities stopping rick (e.g. blocks) go to slot 0 */
            if (ents_entities[0].n) continue;
            e = 0;
            ents_entities[0].c1 = 0;
        }

        /*
         * initialize the entity
         */
        ents_entities[e].mark = m;

        ents_entities[e].flags = get_map_flags(m);
        ents_entities[e].n = map_marks[m].ent;

        /*
         * if entity is to be already running (i.e. not asleep and waiting
         * for some trigger to move), then use LETHALR i.e. restart flag, right
         * from the beginning
         */
        if (ents_entities[e].flags.lethal_at_restart)
            ents_entities[e].n |= ENT_LETHAL;

        ents_entities[e].position.x = map_marks[m].xy & 0xf8;

        y = (uint16_t)((map_marks[m].xy & 0x07) + (map_marks[m].row & 0xf8) - map_frow);
        y <<= 3;
        if (!(ents_entities[e].flags.stoprick))
            y += 3;
        ents_entities[e].position.y = (int16_t) y;

        ents_entities[e].saved = ents_entities[e].position;

        /*ent_ents[e].w0C = 0;*/  /* in ASM code but never used */

        ents_entities[e].dimension.width = ent_entdata[map_marks[m].ent].w;
        ents_entities[e].dimension.height = ent_entdata[map_marks[m].ent].h;
        ents_entities[e].sprbase = ent_entdata[map_marks[m].ent].spr;
        ents_entities[e].sprite = (uint8_t) ent_entdata[map_marks[m].ent].spr;
        ents_entities[e].step_no_i = ent_entdata[map_marks[m].ent].sni;
        ents_entities[e].trigsnd = (uint8_t) ent_entdata[map_marks[m].ent].snd;

        /*
         * FIXME what is this? when all trigger flags are up, then
         * use .sni for sprbase. Why? What is the point? (This is
         * for type 1 and 2 e_them, ...)
         *
         * This also means that as long as sprite has not been
         * recalculated, a wrong value is used. This is normal, see
         * what happens to the falling guy on the right on submap 3:
         * it changes when hitting the ground.
         */
        if (e >= 0x09 && (ents_entities[e].flags.triggered_bomb &&
                          ents_entities[e].flags.triggered_bullet &&
                          ents_entities[e].flags.triggered_stop &&
                          ents_entities[e].flags.triggered_rick))
            ents_entities[e].sprbase = (uint8_t)(ent_entdata[map_marks[m].ent].sni & 0x00ff);

        ents_entities[e].trigger.x = map_marks[m].lt & 0xf8;
        ents_entities[e].latency = (uint8_t)((map_marks[m].lt & 0x07) << 5);  /* <<5 eq *32 */

        ents_entities[e].trigger.y = 3 + 8 * ((map_marks[m].row & 0xf8) - map_frow +
                                              (map_marks[m].lt & 0x07));

        ents_entities[e].c2 = 0;
        ents_entities[e].offsy = 0;
        ents_entities[e].ylow = 0;

        ents_entities[e].front = (ents_entities[e].n <= 18) ? true : false;
    }
}


/*
 * Add a tile-aligned rectangle containing the given rectangle (indicated
 * by its MAP coordinates) to the list of rectangles. Clip the rectangle
 * so it fits into the display zone.
 */
void ent_addrect(pos_t position, dim_t dimension)
{
    pos_t clipped;
    dim_t dim0;

    /* align to tiles */
    clipped.x = position.x & 0xfff8;
    clipped.y = position.y & 0xfff8;
    dim0.width = (uint16_t)(
            (position.x - clipped.x) ? (dimension.width + (position.x - clipped.x)) | 0x0007 : dimension.width);
    dim0.height = (uint16_t)(
            (position.y - clipped.y) ? (dimension.height + (position.y - clipped.y)) | 0x0007 : dimension.height);

    /* clip */
    if (draw_isOutsideScreen(clipped, dim0))    /* do not add if fully clipped */
        return;
    draw_clipToMapScreen(&clipped, &dim0);

    /* get to screen */
    draw_gettoscreen(&clipped);

    /* add rectangle to the list */
    ent_rects = rects_new((uint16_t) clipped.x, (uint16_t) clipped.y, dim0.width, dim0.height, ent_rects);
}


/*
 * Draw all entities onto the frame buffer.
 *
 * ASM 07a4
 *
 * NOTE This may need to be part of draw.c. Also needs better comments,
 * NOTE and probably better rectangles management.
 */
/*
 * background loop : erase all entities that were visible
 */
void ent_backgroundLoop(bool ch3)
{
    uint8_t e;

    for (e = 0; ents_entities[e].n != 0xff; e++) {
        /* if entity was active, then erase it (redraw the map) */
        if (ents_entities[e].prev_n && (ch3 || ents_entities[e].prev_sprite))
            draw_spriteBackground(ents_entities[e].previous);
    }
}


/*
 * foreground loop : draw all entities that are visible
 */
void ent_foregroundLoop(void)
{
    uint8_t e;


    for (e = 0; ents_entities[e].n != 0xff; e++) {
        /*
         * If entity is active now, draw the sprite.
         */
        if (ents_entities[e].n && (game_cheat.xrayvision || ents_entities[e].sprite))
            draw_sprite2(ents_entities[e].sprite,
                         ents_entities[e].position,
                         ents_entities[e].dimension,
                         ents_entities[e].front);
    }

}

/*
 * rectangles loop : figure out which parts of the screen have been
 * impacted and need to be refreshed, then save state
 */
void ent_rectanglesLoop(bool ch3)
{
    uint8_t i;
    uint16_t dx, dy;
    dim_t dimension;

    /* reset rectangles list */
    rects_free(ent_rects);
    ent_rects = NULL;

    for (i = 0; ents_entities[i].n != 0xff; i++) {
        dimension = ents_entities[i].dimension;
        /*
         * If entity was not active before, add a rectangle for the sprite.
         */
        if (ents_entities[i].prev_n && (ch3 || ents_entities[i].prev_sprite)) {
            /* (1) if entity was active and has been drawn ... */
            if (ents_entities[i].n && (game_cheat.xrayvision || ents_entities[i].sprite)) {
                /* (1.1) ... and is still active now and still needs to be drawn, */
                /*       then check if rectangles intersect */
                dx = (uint16_t) abs(ents_entities[i].position.x - ents_entities[i].previous.x);
                dy = (uint16_t) abs(ents_entities[i].position.y - ents_entities[i].previous.y);

                if (dx < dimension.width && dy < dimension.height) {
                    /* (1.1.1) if they do, then create one rectangle */
                    pos_t position;
                    position.x = (ents_entities[i].previous.x < ents_entities[i].position.x)
                                 ? ents_entities[i].previous.x : ents_entities[i].position.x;
                    position.y = (ents_entities[i].previous.y < ents_entities[i].position.y)
                                 ? ents_entities[i].previous.y : ents_entities[i].position.y;
                    dimension.width += dx;
                    dimension.height += dy;
                    ent_addrect(position, dimension);
                } else {
                    /* (1.1.2) else, create two rectangles */
                    ent_addrect(ents_entities[i].position, dimension);
                    ent_addrect(ents_entities[i].previous, dimension);
                }
            } else
                /* (1.2) ... and is not active anymore or does not need to be drawn */
                /*       then create one single rectangle */
                ent_addrect(ents_entities[i].previous, dimension);
        } else if (ents_entities[i].n && (game_cheat.xrayvision || ents_entities[i].sprite)) {
            /* (2) if entity is active and needs to be drawn, */
            /*     then create one rectangle */
            ent_addrect(ents_entities[i].position, dimension);
        }

        /* save state */
        ents_entities[i].previous = ents_entities[i].position;
        ents_entities[i].prev_n = ents_entities[i].n;
        ents_entities[i].prev_sprite = ents_entities[i].sprite;
    }
}

void ent_draw(void)
{
    static bool prev_game_cheat3 = false;

    draw_tilesBank = map_tilesBank;

    ent_backgroundLoop(prev_game_cheat3);

    ent_foregroundLoop();

    ent_rectanglesLoop(prev_game_cheat3);

    prev_game_cheat3 = game_cheat.xrayvision;
}


/*
 * Clear entities previous state
 *
 */
void ent_clprev(void)
{
    uint8_t e;

    for (e = 0; ents_entities[e].n != 0xff; e++)
        ents_entities[e].prev_n = 0;
}


/*
 * Run entities action function
 *
 */
void ent_action(void)
{
    uint8_t e, k;

    for (e = 0; ents_entities[e].n != 0xff; e++) {
        if (ents_entities[e].n) {
            k = (uint8_t)(ents_entities[e].n & 0x7f);
            if (k == 0x47)
                e_them_z_action(e);
            else if (k >= 0x18)
                e_them_t3_action(e);
            else
                ent_actf[k](e);
        }
    }
}


ents_flags_t get_map_flags(uint16_t m)
{
    ents_flags_t flags = {
            .once               = (bool) (map_marks[m].flags & (1u << 0)),
            .stoprick           = (bool) (map_marks[m].flags & (1u << 1)),
            .lethal_at_restart  = (bool) (map_marks[m].flags & (1u << 2)),
            .lethal_at_init     = (bool) (map_marks[m].flags & (1u << 3)),
            .triggered_bomb     = (bool) (map_marks[m].flags & (1u << 4)),
            .triggered_bullet   = (bool) (map_marks[m].flags & (1u << 5)),
            .triggered_stop     = (bool) (map_marks[m].flags & (1u << 6)),
            .triggered_rick     = (bool) (map_marks[m].flags & (1u << 7))
    };

    return flags;
}


/* eof */
