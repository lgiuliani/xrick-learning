/*
 * xrick/src/e_them.c
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
#include <stdlib.h>     /* srand, rand */

#include "system.h"
#include "game.h"
#include "ents.h"

#include "e_rick.h"
#include "e_bomb.h"
#include "e_bullet.h"
#include "maps.h"
#include "util.h"
#include "e_them.h"

typedef enum { TYPE_1A = 0x00, TYPE_1B = 0xff} type_t;

/*
 * prototypes
 */
static bool u_themtest(uint8_t);
static void e_them_gozombie(uint8_t);
static void e_them_t1_action(uint8_t, type_t);
static void e_them_t3_action2(uint8_t);
static bool e_them_t3_action2_trigger(uint8_t);
static void e_them_lethal_checks(uint8_t);
static void e_them_climb(uint8_t);
static void e_them_climbing_not(uint8_t);
static void e_them_xmove(uint8_t);


/*
 * Check if entity boxtests with a lethal e_them i.e. something lethal
 * in slot 0 and 4 to 8.
 *
 * ASM 122E
 *
 * e: entity slot number.
 * ret: TRUE/boxtests, FALSE/not
 */
bool u_themtest(uint8_t e)
{
    uint8_t i;

    if ((ents_entities[0].n & ENT_LETHAL) && u_isEntitiesColliding(e, 0))
        return true;

    for (i = 4; i < 9; i++)
        if ((ents_entities[i].n & ENT_LETHAL) && u_isEntitiesColliding(e, i))
            return true;

    return false;
}


/*
 * Go zombie
 *
 * ASM 237B
 */
void e_them_gozombie(uint8_t e)
{
#define offsx c1
    ents_entities[e].n = 0x47;  /* zombie entity */
    ents_entities[e].front = true;
    ents_entities[e].offsy = -0x0400;
#ifdef ENABLE_SOUND
    syssnd_play(WAV.DIE, 1);
#endif
    game_score += 50;
    if (ents_entities[e].flags.once)
    {
        /* make sure entity won't be activated again */
        map_marks[ents_entities[e].mark].ent |= MAP_MARK_NACT;
    }
    ents_entities[e].offsx = (ents_entities[e].position.x >= 0x80 ? -0x02 : 0x02);
#undef offsx
}


/*
 * Action sub-function for e_them _t1a and _t1b
 *
 * Those two types move horizontally, and fall if they have to.
 * Type 1a moves horizontally over a given distance and then
 * u-turns and repeats; type 1b is more subtle as it does u-turns
 * in order to move horizontally towards rick.
 *
 * ASM 2242
 */
void e_them_t1_action(uint8_t e, type_t type)
{
#define offsx c1
#define step_count c2
    int16_t new_x, new_y;
    uint8_t new_ylow;
    map_flags_t env1;

    /* by default, try vertical move. calculate new y */
    u_calc_new_y(e, &new_y, &new_ylow);

    /* deactivate if outside vertical boundaries */
    /* no need to test zero since e_them _t1a/b don't go up */
    /* FIXME what if they got scrolled out ? */
    if (new_y > SYSVID_WIDTH)
    {
        ents_entities[e].n = 0;
        return;
    }

    /* test environment */
    env1 = u_envtest(ents_entities[e].position.x, new_y, false);

    /* vertical move possible: falling */
    if (!(env1.vertical || env1.solid || env1.superpad || env1.wayup))
    {
        if (env1.lethal)
        {
            /* lethal entities kill e_them */
            e_them_gozombie(e);
            return;
        }
        /* save, cleanup and return */
        ents_entities[e].position.y = new_y;
        ents_entities[e].ylow = new_ylow;
        ents_entities[e].offsy += 0x0080;
        if (ents_entities[e].offsy > 0x0800)
            ents_entities[e].offsy = 0x0800;
        return;
    }

    /* vertical move not possible. calculate new sprite */
    ents_entities[e].sprite = (uint8_t) (ents_entities[e].sprbase
                                         + ent_sprseq[(ents_entities[e].position.x & 0x1c) >> 3]
                                         + (ents_entities[e].offsx < 0 ? 0x03 : 0x00));

    /* reset offsy */
    ents_entities[e].offsy = 0x0080;

    /* align to ground */
    ents_entities[e].position.y &= 0xfff8;
    ents_entities[e].position.y |= 0x0003;

    /* latency: if not zero then decrease and return */
    if (ents_entities[e].latency > 0)
    {
        ents_entities[e].latency--;
        return;
    }

    /* horizontal move. calculate new x */
    if (ents_entities[e].offsx == 0)  /* not supposed to move -> don't */
        return;

    if (ents_entities[e].position.x < 0 || ents_entities[e].position.x > 0xe8)
    {
        /*  U-turn and return if reaching horizontal boundaries */
        ents_entities[e].step_count = 0;
        ents_entities[e].offsx = -ents_entities[e].offsx;
        return;
    }

    /* test environment */
    new_x = ents_entities[e].position.x + ents_entities[e].offsx;
    env1 = u_envtest(new_x, ents_entities[e].position.y, false);
    if (env1.vertical || env1.solid || env1.superpad || env1.wayup)
    {
        /* horizontal move not possible: u-turn and return */
        ents_entities[e].step_count = 0;
        ents_entities[e].offsx = -ents_entities[e].offsx;
        return;
    }

    /* horizontal move possible */
    if (env1.lethal)
    {
        /* lethal entities kill e_them */
        e_them_gozombie(e);
        return;
    }

    /* save */
    ents_entities[e].position.x = new_x;

    /* depending on type, */
    if (type == TYPE_1B)
    {
        /* set direction to move horizontally towards rick */
        if ((ents_entities[e].position.x & 0x1e) != 0x10)  /* prevents too frequent u-turns */
            return;
        ents_entities[e].offsx = (ents_entities[e].position.x < E_RICK_ENT.position.x) ? 0x02 : -0x02;
        return;
    }
    else
    {
        /* set direction according to step counter */
        ents_entities[e].step_count++;
        /* FIXME why trig_x (b16) ?? */
        if ((ents_entities[e].trigger.x >> 1) > ents_entities[e].step_count)
            return;
    }

    /* type is 1A and step counter reached its limit: u-turn */
    ents_entities[e].step_count = 0;
    ents_entities[e].offsx = -ents_entities[e].offsx;
#undef offsx
#undef step_count
}


void e_them_lethal_checks(uint8_t e)
{
    /* lethal entities kill them */
    if (u_themtest(e))
    {
        e_them_gozombie(e);
        return;
    }

    /* bullet kills them */
    if (e_bullet_isHitting(e))
    {
        e_them_gozombie(e);
        return;
    }

    /* bomb kills them */
    if (e_bomb_isExplosing(e))
    {
        e_them_gozombie(e);
        return;
    }

    /* rick stops them */
    if (e_rick_isBlocking(e))
        ents_entities[e].latency = 0x14;

    /* they kill rick */
    if (e_rick_isColliding(e))
        e_rick_gozombie();
}

/*
 * Action function for e_them _t1a type (stays within boundaries)
 *
 * ASM 2452
 */
void e_them_t1a_action(uint8_t e)
{
    e_them_t1_action(e, TYPE_1A);
    e_them_lethal_checks(e);
}


/*
 * Action function for e_them _t1b type (runs for rick)
 *
 * ASM 21CA
 */
void e_them_t1b_action(uint8_t e)
{
    e_them_t1_action(e, TYPE_1B);
    e_them_lethal_checks(e);
}


/*
 * Action function for e_them _z (zombie) type
 *
 * ASM 23B8
 */
void e_them_z_action(uint8_t e)
{
#define offsx c1
    pos_t position;

    position = ents_entities[e].position;

    /* calc new sprite */
    ents_entities[e].sprite = (uint8_t) (ents_entities[e].sprbase + ((position.x & 0x04) ? 0x07 : 0x06));

    /* deactivate if out of vertical boundaries */
    if (position.y < 0 || position.y > SYSVID_WIDTH)
    {
        ents_entities[e].n = 0;
        return;
    }

    /* calc new y & save */
    u_calc_new_y(e,&position.y, &ents_entities[e].ylow);
    ents_entities[e].offsy += 0x0080;

    /* calc new x & save */
    position.x += ents_entities[e].offsx;

    /* must stay within horizontal boundaries */
    if (position.x < 0)
        position.x = 0;
    else if (position.x > 0xe8)
        position.x = 0xe8;

    ents_entities[e].position = position;
#undef offsx
}


/*
 * Action sub-function for e_them _t2.
 *
 * Must document what it does.
 *
 * ASM 2792
 */
void e_them_climb(uint8_t e)
{
    int16_t y, yd;
    map_flags_t env1;

    /* CLIMBING */
    /* latency: if not zero then return */
    if (ents_entities[e].latency > 0) return;

    /* calc new sprite */
    ents_entities[e].sprite = (uint8_t) (ents_entities[e].sprbase + 0x08 +
                                         (((ents_entities[e].position.x ^ ents_entities[e].position.y) & 0x04) ? 1 : 0));

    /* reached rick's level? */
    if ((ents_entities[e].position.y & 0xfe) != (E_RICK_ENT.position.y & 0xfe))
    {
        /* Ymove */
        /* calc new y and test environment */
        yd = ents_entities[e].position.y < E_RICK_ENT.position.y ? 0x02 : -0x02;
        y = ents_entities[e].position.y + yd;
        if (y < 0 || y > SYSVID_WIDTH)
        {
            ents_entities[e].n = 0;
            return;
        }
        env1 = u_envtest(ents_entities[e].position.x, y, false);
        if (env1.solid || env1.superpad || env1.wayup)
        {
            if (yd < 0) /* can't go up */
                e_them_xmove(e);
            else        /* can't go down */
                e_them_climbing_not(e);

            return;
        }
        /* can move */
        ents_entities[e].position.y = y;
        if (env1.vertical || env1.climb)  /* still climbing */
            return;

        e_them_climbing_not(e);
    }
    else
        e_them_xmove(e);
}

#define offsx c2
static void
e_them_xmove(uint8_t e)
{
    int16_t x;
    map_flags_t env1;

    /* calc new x and test environment */
    ents_entities[e].offsx = (ents_entities[e].position.x < E_RICK_ENT.position.x) ? 0x02 : -0x02;
    x = ents_entities[e].position.x + ents_entities[e].offsx;
    env1 = u_envtest(x, ents_entities[e].position.y, false);
    if (env1.solid || env1.superpad || env1.wayup)
        return;
    if (env1.lethal)
    {
        e_them_gozombie(e);
        return;
    }
    ents_entities[e].position.x = x;
    if (env1.vertical || env1.climb)  /* still climbing */
        return;

    e_them_climbing_not(e);
}
#undef offsx

#define flgclmb c1
#define offsx c2
void e_them_climbing_not(uint8_t e)
{
    int16_t new_x, new_y;
    uint8_t new_ylow;
    map_flags_t env1;

    ents_entities[e].flgclmb = false;  /* not climbing */

    /* calc new y (falling) and test environment */
    u_calc_new_y(e, &new_y, &new_ylow);
    env1 = u_envtest(ents_entities[e].position.x, new_y, false);
    if (!(env1.solid || env1.superpad || env1.wayup))
    {
        /* can go there */
        if (env1.lethal)
        {
            e_them_gozombie(e);
            return;
        }
        if (new_y > SYSVID_WIDTH)    /* deactivate if outside */
        {
            ents_entities[e].n = 0;
            return;
        }
        if (!env1.vertical)
        {
            /* save */
            ents_entities[e].position.y = new_y;
            ents_entities[e].ylow = new_ylow;
            ents_entities[e].offsy += 0x0080;
            if (ents_entities[e].offsy > 0x0800)
                ents_entities[e].offsy = 0x0800;
            return;
        }
        if (((ents_entities[e].position.x & 0x07) == 0x04) && (new_y < E_RICK_ENT.position.y))
        {
            ents_entities[e].flgclmb = true;  /* climbing */
            return;
        }
    }

    /* can't go there, or ... */
    ents_entities[e].position.y = (ents_entities[e].position.y & 0xf8) | 0x03;  /* align to ground */
    ents_entities[e].offsy = 0x0100;
    if (ents_entities[e].latency != 0)
        return;

    if (env1.climb &&
            ((ents_entities[e].position.x & 0x0e) == 0x04) &&
            (ents_entities[e].position.y > E_RICK_ENT.position.y))
    {
        ents_entities[e].flgclmb = true;  /* climbing */
        return;
    }

    /* calc new sprite */
    ents_entities[e].sprite = (uint8_t) (ents_entities[e].sprbase +
                                         ent_sprseq[(ents_entities[e].offsx < 0 ? 4 : 0) +
                                             ((ents_entities[e].position.x & 0x0e) >> 3)]);

    /* */
    if (ents_entities[e].offsx == 0)
        ents_entities[e].offsx = 0x02;
    new_x = ents_entities[e].position.x + ents_entities[e].offsx;

    if (new_x < 0xe8)
    {
        env1 = u_envtest(new_x, ents_entities[e].position.y, false);
        if (!(env1.vertical || env1.solid || env1.superpad || env1.wayup))
        {
            ents_entities[e].position.x = new_x;
            if ((new_x & 0x1e) != 0x08)
                return;

            /*
             * Black Magic (tm)
            */
            ents_entities[e].offsx = (rand() % 2) ? -0x02 : 0x02;
            return;
        }
    }

    /* U-turn */
    ents_entities[e].offsx = (ents_entities[e].offsx == 0)? 2 : -ents_entities[e].offsx;

}
#undef offsx
#undef flgclmb


/*
 * Action function for e_them _t2 type
 *
 * ASM 2718
 */
#define flgclmb c1
void
e_them_t2_action(uint8_t e)
{
    /* latency: if not zero then decrease */
    if (ents_entities[e].latency > 0)
        ents_entities[e].latency--;

    /* climbing? */
    if (ents_entities[e].flgclmb == true)
        e_them_climb(e);
    else
        e_them_climbing_not(e);

    e_them_lethal_checks(e);
}
#undef flgclmb

/*
 * Action sub-function for e_them _t3
 *
 * FIXME always starts asleep??
 *
 * Waits until triggered by something, then execute move steps from
 * ent_mvstep with sprite from ent_sprseq. When done, either restart
 * or disappear.
 *
 * Not always lethal ... but if lethal, kills rick.
 *
 * ASM: 255A
 */
void e_them_t3_action2(uint8_t e)
{
#define sproffs c1
#define step_count c2
    uint16_t sprite;
    pos_t new_position;

    while (1)
    {

        /* calc new sprite */
        sprite = ent_sprseq[ents_entities[e].sprbase + ents_entities[e].sproffs];
        ents_entities[e].sprite = (uint8_t) ((sprite == 0xff) ? ent_sprseq[ents_entities[e].sprbase] : sprite);

        if (ents_entities[e].sproffs != 0)    /* awake */
        {

            /* rotate sprseq */
            if (sprite != 0xff)
                ents_entities[e].sproffs++;
            if (ent_sprseq[ents_entities[e].sprbase + ents_entities[e].sproffs] == 0xff)
                ents_entities[e].sproffs = 1;

            if (ents_entities[e].step_count < ent_mvstep[ents_entities[e].step_no].count)
            {
                /*
                 * still running this step: try to increment x and y while
                 * checking that they remain within boudaries. if so, return.
                 * else switch to next step.
                 */
                ents_entities[e].step_count++;
                new_position.x = ents_entities[e].position.x + ent_mvstep[ents_entities[e].step_no].dx;
                new_position.y = ents_entities[e].position.y + ent_mvstep[ents_entities[e].step_no].dy;

                /* check'n save */
                if (new_position.x > 0 && new_position.x < 0xe8 &&
                    new_position.y > 0 && new_position.y < SYSVID_WIDTH)
                {
                    ents_entities[e].position = new_position;
                    return;
                }
            }

            /*
             * step is done, or x or y is outside boundaries. try to
             * switch to next step
             */
            ents_entities[e].step_no++;
            if (ent_mvstep[ents_entities[e].step_no].count != 0xff)
            {
                /* there is a next step: init and loop */
                ents_entities[e].step_count = 0;
            }
            else
            {
                /* there is no next step: restart or deactivate */
                if ((!e_rick_isZombie()) && !(ents_entities[e].flags.once))
                {
                    /* loop this entity */
                    ents_entities[e].sproffs = 0;
                    ents_entities[e].position = ents_entities[e].saved;

                    if (ents_entities[e].position.y < 0 || ents_entities[e].position.y > SYSVID_WIDTH)
                    {
                        ents_entities[e].n = 0;
                        return;
                    }
                    ents_entities[e].n &= ~ENT_LETHAL;
                    if (ents_entities[e].flags.lethal_at_restart)
                        ents_entities[e].n |= ENT_LETHAL;
                }
                else
                {
                    /* deactivate this entity */
                    ents_entities[e].n = 0;
                    return;
                }
            }
        }
        else    /* ent_ents[e].sprseq1 == 0 -- waiting */
        {
            /* not triggered: keep waiting */
            if (!e_them_t3_action2_trigger(e))
                return;

            /* something triggered the entity: wake up */
            /* initialize step counter */
            if (e_rick_isZombie())
                return;
#ifdef ENABLE_SOUND
            /*
            * FIXME the sound should come from a table, there are 10 of them
            * but I dont have the table yet. must rip the data off the game...
            * FIXME is it 8 of them, not 10?
            * FIXME testing below...
            */
            syssnd_play(WAV.ENTITY[(ents_entities[e].trigsnd & 0x1F) - 0x14], 1);
            /*syssnd_play(WAV.ENTITY[0], 1);*/
#endif
            ents_entities[e].n &= ~ENT_LETHAL;
            if (ents_entities[e].flags.lethal_at_init)
                ents_entities[e].n |= ENT_LETHAL;
            ents_entities[e].sproffs = 1;
            ents_entities[e].step_count = 0;
            ents_entities[e].step_no = ents_entities[e].step_no_i;
            return;
        }
    }
#undef step_count
}

bool e_them_t3_action2_trigger(uint8_t e)
{
    /* reacts to rick & wake up if triggered by rick */
    if (ents_entities[e].flags.triggered_rick && e_rick_isTriggering(e))
        return true;

    /* reacts to rick "stop" & wake up if triggered by rick "stop" */
    if (ents_entities[e].flags.triggered_stop && e_rick_isTriggeringWithStop(e))
        return true;

    /* reacts to bullets & wake up if triggered by bullet */
    if (ents_entities[e].flags.triggered_bullet &&  e_bullet_isTriggering(e))
        return true;

    /* reacts to bombs & wake up if triggered by bomb */
    if (ents_entities[e].flags.triggered_bomb &&  e_bomb_isTriggering(e))
        return true;

    return false;
}


/*
 * Action function for e_them _t3 type
 *
 * ASM 2546
 */
void
e_them_t3_action(uint8_t e)
{
    e_them_t3_action2(e);

    /* if lethal, can kill rick */
    if ((ents_entities[e].n & ENT_LETHAL) &&
            e_rick_isColliding(e) && !e_rick_isZombie())    /* CALL 1130 */
    {
        e_rick_gozombie();
    }
}

/* eof */
