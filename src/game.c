/*
 * xrick/src/game.c
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

#include "draw.h"
#include "maps.h"
#include "e_rick.h"
#include "screens.h"
#include "scroller.h"
#include "control.h"
#include "sysvid.h"


/*
 * global vars
 */
rect_t *game_rects = NULL;

uint8_t game_lives = 0;
uint8_t game_bombs = 0;
uint8_t game_bullets = 0;
uint32_t game_score = 0;

uint16_t game_map = 0;
uint16_t game_submap = 0;

bool game_chsm = false;

game_cheat_t game_cheat ={
    .unlimited = false,
    .immortal = false,
    .xrayvision = false
};

hscore_t game_hscores[8] =
{
    { 8000, "SIMES@@@@@" },
    { 7000, "JAYNE@@@@@" },
    { 6000, "DANGERSTU@" },
    { 5000, "KEN@@@@@@@" },
    { 4000, "ROB@N@BOB@" },
    { 3000, "TELLY@@@@@" },
    { 2000, "NOBBY@@@@@" },
    { 1000, "JEZEBEL@@@" }
};


/*
 * local vars
 */
static uint8_t game_period, saved_game_period = 0; /* time between each frame, in millisecond */
static bool game_waitevt = false; /* wait for events (TRUE, FALSE) */
static uint8_t isave_frow;

typedef enum
{
    INTRO_MAIN, INTRO_MAP,
    PAUSE_PRESSED1, PAUSED, PAUSE_PRESSED2,
    PLAY3,
    CHAIN_MAP,
    SCROLL_UP, SCROLL_DOWN,
    RESTART, GAMEOVER, GETNAME
} game_state_t;
static game_state_t game_state;

#ifdef ENABLE_SOUND
static sound_t *music_snd;
#endif

/*
 * prototypes
 */
static void timer(void);
static void frame(void);
static game_state_t init_game(void);
static game_state_t init_buffer(void);
static game_state_t pause_check(void);
static game_state_t play2(void);
static game_state_t play3(void);
static game_state_t chain_submap(void);
static game_state_t chain_map(void);
static game_state_t chain_end(void);
static void restart(void);
static void isave(void);
static void irestore(void);
static void loaddata(void);
static void freedata(void);
static void game_stopmusic(void);


/*
 * Cheats
 */
void
game_toggleCheat(cheat_t cheat_type)
{
    if (game_state != INTRO_MAIN && game_state != INTRO_MAP &&
            game_state != GAMEOVER && game_state != GETNAME)
    {
        switch (cheat_type)
        {
        case CHEAT1:
            game_cheat.unlimited = !game_cheat.unlimited;
            game_lives = GAME_LIVES_INIT;
            game_bombs = GAME_BOMBS_INIT;
            game_bullets = GAME_BULLETS_INIT;
            break;
        case CHEAT2:
            game_cheat.immortal = !game_cheat.immortal;
            break;
        case CHEAT3:
            game_cheat.xrayvision = !game_cheat.xrayvision;
            break;
        }
        draw_infos();
        /* FIXME this should probably only raise a flag ... */
        /* plus we only need to update INFORECT not the whole screen */
        sysvid_update(&draw_SCREENRECT);
    }
}

#ifdef ENABLE_SOUND
/*
 * Music
 */
void
game_setmusic(char *name, int8_t loop)
{
    if (music_snd)
        game_stopmusic();
    music_snd = syssnd_load(name);
    if (music_snd)
    {
        music_snd->dispose = true; /* music is always "fire and forget" */
        syssnd_play(music_snd, loop);
    }
}

void
game_stopmusic(void)
{
    syssnd_stopsound(music_snd);
    music_snd = NULL;
}
#endif

inline void
timer(void)
{
    static uint32_t tm = 0;
    uint32_t tmx;

    /* timer */
    tmx = tm;
    tm = sys_gettime();
    tmx = tm - tmx;

    if (tmx < game_period)
        sys_sleep((uint32_t)game_period - tmx);
}

/*
 * Main loop
 */
void
game_run(void)
{

    loaddata(); /* load cached data */

    game_period = (uint8_t) (sysarg_args.period ? sysarg_args.period : GAME_PERIOD);
    game_state = INTRO_MAIN;

    /* main loop */
    while (true)
    {

        timer();

        /* video */
        sysvid_update(game_rects);
        //draw_STATUSRECT.next = NULL;  /* FIXME freerects should handle this */

        /* sound */
        /*snd_mix();*/

        /* events */
        if (game_waitevt)
            sysevt_wait();  /* wait for an event */
        else
            sysevt_poll();  /* process events (non-blocking) */

        /* request to exit the game */
        if (control.exit)
            break;

        frame();
    }

    freedata(); /* free cached data */
}

/*
 * Prepare frame
 *
 * This function loops forever: use 'return' when a frame is ready.
 * When returning, game_rects must contain every parts of the buffer
 * that have been modified.
 */
static void
frame(void)
{
    switch (game_state)
    {
    case INTRO_MAIN:
        if (screen_introMain() == DONE)
            game_state = init_game();
        return;
    case INTRO_MAP:
        if (screen_introMap() == DONE)
            game_state = init_buffer();
        return;
    case PLAY3:
        game_state = play3();
        return;
    case CHAIN_MAP:
        game_state = chain_map();
        return;
    case SCROLL_UP:
        if (scroll_up() == DONE)
            game_state = pause_check();
        return;
    case SCROLL_DOWN:
        if (scroll_down() == DONE)
            game_state = pause_check();
        return;
    case RESTART:
        restart();
        game_state = init_buffer();
        return;
    case GAMEOVER:
        if (screen_gameover() == DONE)
            game_state = GETNAME;
        return;
    case GETNAME:
        if (screen_getname() == DONE)
            game_state = INTRO_MAIN;
        return;
    case PAUSE_PRESSED1:
        if (!control.pause)
            game_state = PAUSED;
        return;
    case PAUSED:
        if (control.pause)
            game_state = PAUSE_PRESSED2;
        return;
    case PAUSE_PRESSED2:
        if (control.pause)
            return;

        game_waitevt = false;
        screen_pause(false);
#ifdef ENABLE_SOUND
        syssnd_pause(false, false);
#endif
        game_state = play2();
        return;
    }
}


/*
 * Init buffer
 */
static game_state_t
init_buffer(void)
{
    sysvid_clear();                 /* clear buffer */
    draw_map();                     /* draw the map onto the buffer */
    draw_drawStatus();              /* draw the status bar onto the buffer */
    game_rects = &draw_SCREENRECT;  /* request full buffer refresh */
    return pause_check();
}


/*
 * play1
 *
 */
static game_state_t
pause_check(void)
{
    if (!control.pause && control_active)
        return play2();

    /* Pause */
#ifdef ENABLE_SOUND
    syssnd_pause(true, false);
#endif

    game_waitevt = true;
    screen_pause(true);

    if (control.pause)
        return PAUSE_PRESSED1;

    return PAUSED;
}

/*
 * play2
 *
 */
static game_state_t
play2(void)
{
    /* request to end the game */
    if (control.terminate)
        return GAMEOVER;

    ent_action();      /* run entities */
    if (e_rick_isDead())
    {
        if (game_cheat.unlimited)
            return RESTART;

        game_lives--;
        if (game_lives != 0)
            return RESTART;

        return GAMEOVER;
    }

    if (game_chsm)  /* request to chain to next submap */
        return chain_submap();

    return PLAY3;
}

/*
 * play3
 *
 */
static game_state_t
play3(void)
{
    static rect_t *r;

    draw_clearStatus();  /* clear the status bar */
    draw_drawStatus();   /* draw the status bar onto the buffer*/
    ent_draw();          /* draw all entities onto the buffer */

    r = &draw_STATUSRECT;
    r->next = ent_rects;    /* refresh status bar too */
    game_rects = r;         /* take care to cleanup draw_STATUSRECT->next later! */

    if (!e_rick_isZombie())    /* need to scroll ? */
    {
        if (E_RICK_ENT.position.y >= 0xCC)
            return SCROLL_UP;
        if (E_RICK_ENT.position.y <= 0x60)
            return SCROLL_DOWN;
    }

    return pause_check();
}


static game_state_t
chain_submap(void)
{
    if (map_chain())
        return chain_end();

    game_bullets = GAME_BULLETS_INIT;
    game_bombs = GAME_BOMBS_INIT;
    game_map++;

    return CHAIN_MAP;
}

static game_state_t
chain_map(void)
{
    if (screen_introMap() == RUNNING)
        return game_state;

    if (game_map < 0x04)    /* Initialize game */
    {
        map_frow = (uint8_t)map_maps[game_map].row;
        game_submap = map_maps[game_map].submap;
        e_rick_init();
        return chain_end();
    }
    /* *else* reached end of game */
    sysarg_args.map = 0;
    sysarg_args.submap = 0;

    return GAMEOVER;
}

static game_state_t
chain_end(void)
{
    map_init();           /* initialize the map */
    isave();              /* save data in case of a restart */
    ent_clprev();         /* cleanup entities */
    return init_buffer();   /* request full screen refresh */
}


/*
 * Initialize the game
 */
static game_state_t
init_game(void)
{
    game_lives = GAME_LIVES_INIT;
    game_bombs = GAME_BOMBS_INIT;
    game_bullets = GAME_BULLETS_INIT;
    game_score = 0;

    game_map = (uint16_t) sysarg_args.map;

    if (sysarg_args.submap == 0)
    {
        game_submap = map_maps[game_map].submap;
        map_frow = (uint8_t)map_maps[game_map].row;
    }
    else
    {
        game_submap = (uint16_t) sysarg_args.submap;
        map_frow = map_find_frow(game_submap);
    }

    e_rick_init();

    ents_entities[ENT_ENTSNUM].n = 0xFF;

    map_resetMarks();

    map_init();
    isave();

    return INTRO_MAP;
}


/*
 * restart
 *
 */
static void
restart(void)
{
    game_bullets = GAME_BULLETS_INIT;
    game_bombs = GAME_BOMBS_INIT;

    e_rick_init();

    irestore();
    map_init();
    isave();

    ent_clprev();
}


/*
 * isave (0bbb)
 *
 */
static void
isave(void)
{
    e_rick_save();
    isave_frow = map_frow;
}


/*
 * irestore (0bdc)
 *
 */
static void
irestore(void)
{
    e_rick_restore();
    map_frow = isave_frow;
}

/*
 *
 */
static void
loaddata()
{
#ifdef ENABLE_SOUND
    /*
     * Cache sounds
     *
     * tune[0-5].wav not cached
     */
    WAV.GAMEOVER = syssnd_load("sounds/gameover.wav");
    WAV.SBONUS2 = syssnd_load("sounds/sbonus2.wav");
    WAV.BULLET = syssnd_load("sounds/bullet.wav");
    WAV.BOMBSHHT = syssnd_load("sounds/bombshht.wav");
    WAV.EXPLODE = syssnd_load("sounds/explode.wav");
    WAV.STICK = syssnd_load("sounds/stick.wav");
    WAV.WALK = syssnd_load("sounds/walk.wav");
    WAV.CRAWL = syssnd_load("sounds/crawl.wav");
    WAV.JUMP = syssnd_load("sounds/jump.wav");
    WAV.PAD = syssnd_load("sounds/pad.wav");
    WAV.BOX = syssnd_load("sounds/box.wav");
    WAV.BONUS = syssnd_load("sounds/bonus.wav");
    WAV.SBONUS1 = syssnd_load("sounds/sbonus1.wav");
    WAV.DIE = syssnd_load("sounds/die.wav");
    WAV.ENTITY[0] = syssnd_load("sounds/ent0.wav");
    WAV.ENTITY[1] = syssnd_load("sounds/ent1.wav");
    WAV.ENTITY[2] = syssnd_load("sounds/ent2.wav");
    WAV.ENTITY[3] = syssnd_load("sounds/ent3.wav");
    WAV.ENTITY[4] = syssnd_load("sounds/ent4.wav");
    WAV.ENTITY[5] = syssnd_load("sounds/ent5.wav");
    WAV.ENTITY[6] = syssnd_load("sounds/ent6.wav");
    WAV.ENTITY[7] = syssnd_load("sounds/ent7.wav");
    WAV.ENTITY[8] = syssnd_load("sounds/ent8.wav");
#endif
}

/*
 *
 */
static void
freedata()
{
#ifdef ENABLE_SOUND
    syssnd_stopall();
    syssnd_free(WAV.GAMEOVER);
    syssnd_free(WAV.SBONUS2);
    syssnd_free(WAV.BULLET);
    syssnd_free(WAV.BOMBSHHT);
    syssnd_free(WAV.EXPLODE);
    syssnd_free(WAV.STICK);
    syssnd_free(WAV.WALK);
    syssnd_free(WAV.CRAWL);
    syssnd_free(WAV.JUMP);
    syssnd_free(WAV.PAD);
    syssnd_free(WAV.BOX);
    syssnd_free(WAV.BONUS);
    syssnd_free(WAV.SBONUS1);
    syssnd_free(WAV.DIE);
    syssnd_free(WAV.ENTITY[0]);
    syssnd_free(WAV.ENTITY[1]);
    syssnd_free(WAV.ENTITY[2]);
    syssnd_free(WAV.ENTITY[3]);
    syssnd_free(WAV.ENTITY[4]);
    syssnd_free(WAV.ENTITY[5]);
    syssnd_free(WAV.ENTITY[6]);
    syssnd_free(WAV.ENTITY[7]);
    syssnd_free(WAV.ENTITY[8]);
#endif
}

void game_change_period(uint8_t value)
{
    saved_game_period = game_period; /* save period, */
    game_period = value;    /* and use our own */
}

void game_restore_period(void)
{
    game_period = saved_game_period;
}
/* eof */
