/*
 * xrick/src/e_rick.c
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

#include "e_bullet.h"
#include "e_bomb.h"
#include "control.h"
#include "maps.h"
#include "util.h"

/*
 * Static Prototypes
 */
static inline void e_rick_z_action(void);
static inline void e_rick_stop(void);
static inline void e_rick_shoot(void);
static inline void e_rick_pose_bomb(void);
static inline void e_rick_climb(void);
static inline bool e_rick_Previous_Submap(int16_t);
static inline bool e_rick_Next_Submap(int16_t);
static inline void e_rick_walk(void);
static inline void e_rick_subaction(void);
static inline pos_t e_rick_stop_position(void);

/*
 * local vars
 */
static rick_state e_rick_state = E_RICK_STAND;
static bool shoot_trigger = false;
static dir_t e_rick_dir = RIGHT;

typedef struct {
    pos_t position;
    bool crawl;
} save_t;
static save_t save;

/*
 * Tigger box test
 *
 */
bool
e_rick_isTriggering(uint8_t e) {
    pos_t position = E_RICK_ENT.position;
    position.x += 0x0C;
    position.y += 0X0A;

    return (u_trigbox(e, position));
}

/*
 * Stop test
 *
 */
bool
e_rick_isBlocking(uint8_t e) {
    return ((e_rick_state == E_RICK_STOP) && u_isEntityCollidePoint(e, e_rick_stop_position()));
}

bool
e_rick_isTriggeringWithStop(uint8_t e) {
    return ((e_rick_state == E_RICK_STOP) && u_trigbox(e, e_rick_stop_position()));
}

pos_t e_rick_stop_position(void) {
    pos_t stop_pos = E_RICK_ENT.position;

    if (control.right)
        stop_pos.x += E_RICK_ENT.dimension.width - 1;

    stop_pos.y += 0x000E;

    return stop_pos;
}

/*
 * Box test
 *
 * ASM 113E (based on)
 *
 * e: entity to test against (corresponds to SI in asm code -- here DI
 *    is assumed to point to rick).
 * ret: TRUE/intersect, FALSE/not.
 */
bool
e_rick_isColliding(uint8_t e) {
    ent_t entity = ents_entities[e];
    /*
     * rick: x+0x05 to x+0x11, y+[0x08 if rick's crawling] to y+0x14
     * entity: x to x+w, y to y+h
     */
    if (E_RICK_ENT.position.x + 0x11 < entity.position.x ||
        E_RICK_ENT.position.x + 0x05 > entity.position.x + entity.dimension.width ||
        E_RICK_ENT.position.y + 0x14 < entity.position.y ||
        E_RICK_ENT.position.y + ((e_rick_state == E_RICK_CRAWL) ? 0x08 : 0x00) >
        entity.position.y + entity.dimension.height - 1)
        return false;

    return true;
}

/*
 * Go zombie
 *
 * ASM 1851
 */
void
e_rick_gozombie(void) {
    if (game_cheat.immortal)
        return;

    /* already zombie? */
    if (e_rick_state == E_RICK_ZOMBIE)
        return;

#ifdef ENABLE_SOUND
    syssnd_play(WAV.DIE, 1);
#endif

    e_rick_state = E_RICK_ZOMBIE;
    E_RICK_ENT.offsy = -0x0400;
    E_RICK_ENT.offsx = (E_RICK_ENT.position.x > 0x80) ? -0x03 : 0x03;
    E_RICK_ENT.ylow = 0;
    E_RICK_ENT.front = true;
}

/*
 * Action sub-function for e_rick when zombie
 *
 * ASM 17DC
 */
void e_rick_z_action(void) {
    /* sprite */
    E_RICK_ENT.sprite = (E_RICK_ENT.position.x & 0x04) ? 0x1A : 0x19;

    /* x */
    E_RICK_ENT.position.x += E_RICK_ENT.offsx;

    /* y */
    u_calc_new_y(E_RICK_NO, &E_RICK_ENT.position.y, &E_RICK_ENT.ylow);
    E_RICK_ENT.offsy += 0x80;

    /* dead when out of screen */
    if (E_RICK_ENT.position.y < 0 || E_RICK_ENT.position.y > SYSVID_WIDTH)
        e_rick_state = E_RICK_DEAD;
}


/*
 * Rick stops
 *
 */
void e_rick_stop(void) {
    e_rick_state = E_RICK_STOP;

    if (control.right)
        e_rick_dir = RIGHT;
    else
        e_rick_dir = LEFT;
}

/*
 * Rick fires
 *
 */
void e_rick_shoot(void) {
    e_rick_state = E_RICK_SHOOT;
    /* not an automatic gun: shoot once only */
    if (shoot_trigger)
        return;
    shoot_trigger = true;

    /* having already a bullet in the air */
    if (E_BULLET_ENT.n != 0)
        return;


    /* Use a bullet, if any available */
    if (!game_bullets)
        return;
    if (!game_cheat.unlimited)
        game_bullets--;
    /* initialize bullet */
    e_bullet_init(E_RICK_ENT.position, e_rick_dir);
}

/*
 * Rick poses a bomb
 *
 */
void e_rick_pose_bomb(void) {
    /* already a bomb ticking ... that's enough */
    if (E_BOMB_ENT.n)
        return;
    /* else use a bomb, if any available */
    if (!game_bombs)
        return;
    if (!game_cheat.unlimited)
        game_bombs--;
    /* initialize bomb */
    e_bomb_init(E_RICK_ENT.position);
}

/*
 * Rick is climbing
 *
 */
void e_rick_climb(void) {
    map_flags_t env1;
    pos_t new_position;

    /* no move: reset seq */
    if (!(control.up || control.down || control.left || control.right)) {
        E_RICK_ENT.seq = 0;
        return;
    }

    new_position = E_RICK_ENT.position;

    if (control.up)
        new_position.y -= 0x02;
    else if (control.down)
        new_position.y += 0x02;

    if (control.left)
        new_position.x -= 0x02;
    else if (control.right)
        new_position.x += 0x02;

    env1 = u_envtest(new_position.x, new_position.y, false);

    if ((env1.solid || env1.superpad || env1.wayup) && !control.up) {
        /* Reach floor */
        e_rick_state = E_RICK_STAND;
        return;
    }

    if (!(env1.solid || env1.superpad || env1.wayup) || env1.wayup) {
        /* ok to move, save */
        E_RICK_ENT.position = new_position;
        if (env1.lethal) {
            e_rick_gozombie();
            return;
        }

        if (!(env1.vertical  || env1.climb)) {
            /* reached end of climb zone */
            E_RICK_ENT.offsy = (control.up) ? -0x0300 : 0x0100;
#ifdef ENABLE_SOUND
            if (control.up)
                syssnd_play(WAV.JUMP, 1);
#endif
            e_rick_state = E_RICK_STAND;
        }
    }
}

//static inline void
//e_rick_climb(void)
//{
//    uint8_t env0, env1;
//    pos_t new_position;
//
//    if (control.up || control.down)
//    {
//        /* up-down: calc new y and test environment */
//        new_position = E_RICK_ENT.position;
//        new_position.y += (control.up) ? -0x02 : 0x02;
//        u_envtest(new_position.x, new_position.y,false, &env0, &env1);
//
//        if (env1 & (MAP_EFLG_SOLID|MAP_EFLG_SPAD|MAP_EFLG_WAYUP) &&
//                !(control.up))
//        {
//            /* Reach floor */
//            e_rick_state = E_RICK_STAND;
//            return;
//        }
//        if (!(env1 & (MAP_EFLG_SOLID|MAP_EFLG_SPAD|MAP_EFLG_WAYUP)) ||
//                (env1 & MAP_EFLG_WAYUP))
//        {
//            /* ok to move, save */
//            E_RICK_ENT.position = new_position;
//            if (env1 & MAP_EFLG_LETHAL)
//            {
//                e_rick_gozombie();
//                return;
//            }
//            if (!(env1 & (MAP_EFLG_VERT|MAP_EFLG_CLIMB)))
//            {
//                /* reached end of climb zone */
//                offsy = (control.up) ? -0x0300: 0x0100;
//#ifdef ENABLE_SOUND
//                if (control.up)
//                    syssnd_play(WAV.JUMP, 1);
//#endif
//                e_rick_state = E_RICK_STAND;
//                return;
//            }
//        }
//    }
//
//    if (control.left || control.right)
//    {
//        /* left-right: calc new x and test environment */
//        new_position = E_RICK_ENT.position;
//        new_position.x += (control.left) ? -0x02 : 0x02;
//        u_envtest(new_position.x, new_position.y, false, &env0, &env1);
//
//        if (env1 & (MAP_EFLG_SOLID|MAP_EFLG_SPAD))
//            return;
//        E_RICK_ENT.position = new_position;
//        if (env1 & MAP_EFLG_LETHAL)
//        {
//            e_rick_gozombie();
//            return;
//        }
//
//        if (!(env1 & (MAP_EFLG_VERT|MAP_EFLG_CLIMB)))
//        {
//            if (control.up)
//                offsy = -0x0300;
//
//            e_rick_state = E_RICK_STAND;
//            return;
//        }
//    }
//
//    /* no move: reset seq */
//    if (!(control.up || control.down || control.left || control.right))
//        seq = 0;
//}

bool e_rick_Previous_Submap(int16_t x) {
    if (x < 0)    /* prev submap */
    {
        game_chsm = true;
        E_RICK_ENT.position.x = 0xe2;
    }

    return game_chsm;
}

bool e_rick_Next_Submap(int16_t x) {
    if (x >= 0xe8)    /* next submap */
    {
        game_chsm = true;
        E_RICK_ENT.position.x = 0x04;
    }

    return game_chsm;
}

void e_rick_walk(void) {
    map_flags_t env1;
    int16_t x;

    /*
    * HORIZONTAL MOVE
    */
    /* should move? */
    if (control.left)    /* move left */
    {
        x = E_RICK_ENT.position.x - 0x02;
        e_rick_dir = LEFT;
        if (e_rick_Previous_Submap(x))
            return;
    } else if (control.right)     /* move right */
    {
        x = E_RICK_ENT.position.x + 0x02;
        e_rick_dir = RIGHT;
        if (e_rick_Next_Submap(x))
            return;
    } else {
        E_RICK_ENT.seq = 2; /* no: reset seq and return */
        return;
    }

    /* still within this map: test environment */
    env1 = u_envtest(x, E_RICK_ENT.position.y, (e_rick_state == E_RICK_CRAWL));

    /* is-it possible to move ? */
    if (env1.solid || env1.superpad || env1.wayup)
        return;

    /* save x-position */
    E_RICK_ENT.position.x = x;

    if (env1.lethal)
        e_rick_gozombie();
}


/*
 * Action sub-function for e_rick.
 *
 * ASM 13BE
 */
void e_rick_subaction(void) {
    map_flags_t env1;
    int16_t new_y;
    uint8_t new_ylow;

    /* Reset states that are temporary */
    if (e_rick_state == E_RICK_STOP || e_rick_state == E_RICK_SHOOT || e_rick_state == E_RICK_JUMP)
        e_rick_state = E_RICK_STAND;

    /* climbing? */
    if (e_rick_state == E_RICK_CLIMB) {
        e_rick_climb();
        return;
    }

    /*
    * NOT CLIMBING
    */
    /* calc y */
    u_calc_new_y(E_RICK_NO, &new_y, &new_ylow);

    /* test environment */
    env1 = u_envtest(E_RICK_ENT.position.x, new_y, (e_rick_state == E_RICK_CRAWL));
    /* stand up, if possible */
    if ((e_rick_state == E_RICK_CRAWL) && !u_envtestcrawl(E_RICK_ENT.position.x, new_y))
        e_rick_state = E_RICK_STAND;

    /* can move vertically? */
    if (E_RICK_ENT.offsy < 0 && (env1.vertical || env1.solid || env1.superpad)) {
        /* NO VERTICAL MOVE */
        /* not climbing + trying to go _up_ not possible -> hit the roof */
        e_rick_state = E_RICK_JUMP;  /* fall back to the ground */
        E_RICK_ENT.position.y &= 0xF8;
        E_RICK_ENT.offsy = 0;
        E_RICK_ENT.ylow = 0;
    } else if (E_RICK_ENT.offsy > 0 && (env1.vertical || env1.solid || env1.superpad || env1.wayup)) {
        /* else: not climbing + trying to go _down_ not possible -> standing */
        /* align to ground */
        E_RICK_ENT.position.y &= 0xF8;
        E_RICK_ENT.position.y |= 0x03;
        E_RICK_ENT.ylow = 0;

        /* standing on a super pad? */
        if (env1.superpad && E_RICK_ENT.offsy >= 0x0200) {
            E_RICK_ENT.offsy = (control.up) ? 0xf800 : 0x00fe - E_RICK_ENT.offsy;
#ifdef ENABLE_SOUND
            syssnd_play(WAV.PAD, 1);
#endif
            e_rick_walk();
            return;
        }

        E_RICK_ENT.offsy = 0x0100;  /* reset*/

        /* standing. firing ? */
        if ((e_rick_state != E_RICK_CRAWL) && (control.fire)) {
            if (control.left || control.right)  /* stop */
                e_rick_stop();
            else if (control.down)              /* bomb */
                e_rick_pose_bomb();
            else if (control.up)                /* bullet */
                e_rick_shoot();
            else
                shoot_trigger = false; /* not shooting means trigger is released */
            E_RICK_ENT.seq = 0; /* reset */
            return;
        }
        shoot_trigger = false; /* not shooting means trigger is released */

        /*
         * NOT FIRING
         */
        if ((e_rick_state != E_RICK_CRAWL) && (control.up))   /* jump or climb */
        {
            if (env1.climb)      /* climb */
            {
                e_rick_state = E_RICK_CLIMB;
                return;
            } else {
                /* jump */
                e_rick_state = E_RICK_JUMP;
                E_RICK_ENT.offsy = -0x0580;
#ifdef ENABLE_SOUND
                syssnd_play(WAV.JUMP, 1);
#endif
            }
        } else if (control.down)    /* crawl or climb */
        {
            if (env1.vertical &&                        /* can go down */
                !(control.left || control.right) &&     /* + not moving horizontaly */
                (E_RICK_ENT.position.x & 0x1f) < 0x0a)  /* + aligned -> climb */
            {
                E_RICK_ENT.position.x = (E_RICK_ENT.position.x & 0xf0) | 0x04;
                e_rick_state = E_RICK_CLIMB;
            } else                            /* crawl */
            {
                e_rick_state = E_RICK_CRAWL;
            }
        }
    } else {

        /* killed? */
        if (env1.lethal) {
            e_rick_gozombie();
            return;
        }

        /* VERTICAL MOVE  */

        /* save */
        E_RICK_ENT.position.y = new_y;
        E_RICK_ENT.ylow = new_ylow;

        /* climb? */
        if (env1.climb && (control.up || control.down)) {
            E_RICK_ENT.offsy = 0x0100;
            e_rick_state = E_RICK_CLIMB;
            return;
        }
        /* fall */
        E_RICK_ENT.offsy += 0x0080;
        if (E_RICK_ENT.offsy > 0x0800) {
            E_RICK_ENT.offsy = 0x0800;
            E_RICK_ENT.ylow = 0;
        }

        /* If not crawling, is there room before jump ? */
        if (e_rick_state != E_RICK_CRAWL || !u_envtestcrawl(E_RICK_ENT.position.x, E_RICK_ENT.position.y))
            e_rick_state = E_RICK_JUMP;
    }

    e_rick_walk();
}

/*
 * Action function for e_rick
 *
 * ASM 12CA
 */
void
e_rick_action(UNUSED(uint8_t e)) {
    static bool stop_trigger = false; /* is this the most elegant way? */

    /* if zombie or dead, nothing to do return */
    if (e_rick_state != E_RICK_ZOMBIE && e_rick_state != E_RICK_DEAD)
        e_rick_subaction();

    switch (e_rick_state) {
        case E_RICK_DEAD:
            break;
        case E_RICK_ZOMBIE:
            e_rick_z_action();
            break;
        case E_RICK_STOP:
            E_RICK_ENT.sprite = (e_rick_dir == RIGHT) ? 0x0B : 0x17;
#ifdef ENABLE_SOUND
            if (!stop_trigger) {
                syssnd_play(WAV.STICK, 1);
                stop_trigger = true;
            }
#endif
            break;
        case E_RICK_SHOOT:
            E_RICK_ENT.sprite = (e_rick_dir == RIGHT) ? 0x0A : 0x16;
            break;
        case E_RICK_CLIMB:
            E_RICK_ENT.sprite = (((E_RICK_ENT.position.x ^ E_RICK_ENT.position.y) & 0x04) ? 0x18 : 0x0c);
#ifdef ENABLE_SOUND
            E_RICK_ENT.seq = (E_RICK_ENT.seq + 1) & 0x03;
            if (E_RICK_ENT.seq == 0)
                syssnd_play(WAV.WALK, 1);
#endif
            break;
        case E_RICK_CRAWL:
            E_RICK_ENT.sprite = (e_rick_dir == RIGHT) ? 0x07 : 0x13;
            if (E_RICK_ENT.position.x & 0x04) E_RICK_ENT.sprite++;
#ifdef ENABLE_SOUND
            E_RICK_ENT.seq = (E_RICK_ENT.seq + 1) & 0x03;
            if (E_RICK_ENT.seq == 0) syssnd_play(WAV.CRAWL, 1);
#endif
            break;
        case E_RICK_JUMP:
            E_RICK_ENT.sprite = (e_rick_dir == RIGHT) ? 0x06 : 0x15;
            break;
        case E_RICK_STAND:
            stop_trigger = false; /* reset stop trigger */
            E_RICK_ENT.seq++;

            if (E_RICK_ENT.seq >= 0x14) {
#ifdef ENABLE_SOUND
                syssnd_play(WAV.WALK, 1);
#endif
                E_RICK_ENT.seq = 0x04;
            }
#ifdef ENABLE_SOUND
            else if (E_RICK_ENT.seq == 0x0C)
                syssnd_play(WAV.WALK, 1);
#endif
            E_RICK_ENT.sprite = (E_RICK_ENT.seq >> 2) + ((e_rick_dir == RIGHT) ? 0x01 : 0x0D);
            break;
    }
}

/*
 * Initialise Rick
 *
 *
 */
void e_rick_init(void) {
    e_rick_state = E_RICK_STAND;
    e_rick_dir = RIGHT;
    E_RICK_ENT.position = map_maps[game_map].initial_position;
    E_RICK_ENT.dimension.width = 0x18;
    E_RICK_ENT.dimension.height = 0x15;
    E_RICK_ENT.n = 0x01;
    E_RICK_ENT.sprite = 0x01;
    E_RICK_ENT.front = true;
}

/*
 * Save status
 *
 * ASM part of 0x0BBB
 */
void e_rick_save(void) {
    save.position = E_RICK_ENT.position;
    save.crawl = (e_rick_state == E_RICK_CRAWL);
    /* FIXME
     * save_C0 = E_RICK_ENT.b0C;
     * plus some 6DBC stuff?
     */
}


/*
 * Restore status
 *
 * ASM part of 0x0BDC
 */
void e_rick_restore(void) {
    E_RICK_ENT.position = save.position;
    E_RICK_ENT.front = true;
    e_rick_state = save.crawl ? E_RICK_CRAWL : E_RICK_STAND;
    /* FIXME
     * E_RICK_ENT.b0C = save_C0;
     * plus some 6DBC stuff?
     */
}

bool e_rick_isConnect(uint16_t c) {
    return (map_connect[c].dir == e_rick_dir);
}

bool e_rick_isZombie(void) {
    return (e_rick_state == E_RICK_ZOMBIE);
}

bool e_rick_isDead(void) {
    return (e_rick_state == E_RICK_DEAD);
}

/* eof */
