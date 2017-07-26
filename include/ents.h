/*
 * xrick/include/ents.h
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

#ifndef ENTS_H
#define ENTS_H

#include "system.h"
#include "rects.h"

#define ENT_NBR_ENTDATA 0x4a
#define ENT_NBR_SPRSEQ 0x88
#define ENT_NBR_MVSTEP 0x310

#define ENT_ENTSNUM 0x0c

/*
 * flags for ent_ents[e].n  ("yes" when set)
 *
 * ENT_LETHAL: is entity lethal?
 */
extern const uint8_t ENT_LETHAL;

/*
 * Bit field structure flags for ent_ents[e].flag  ("true" when set)
 */
typedef struct
{
    bool once:1;                /* should the entity run once only? */
    bool stoprick:1;            /* does the entity stops rick (and goes to slot zero)? */
    bool lethal_at_restart:1;   /* is entity lethal when restarting? */
    bool lethal_at_init:1;      /* is entity initially lethal? */
    bool triggered_bomb:1;      /* can entity be triggered by a bomb? */
    bool triggered_bullet:1;    /* can entity be triggered by a bullet? */
    bool triggered_stop:1;      /* can entity be triggered by rick stop? */
    bool triggered_rick:1;      /* can entity be triggered by rick? */
} ents_flags_t;


typedef struct
{
    uint8_t n;          /* b00 */
    uint8_t sprite;     /* b08 - sprite number */
    /*uint8_t b01;*/    /* b01 in ASM code but never used */
    pos_t position;     /* b02, w04 - position */
    /*uint16_t w0C;*/   /* w0C in ASM code but never used */
    dim_t dimension;    /* b0E - width, b10 - height */
    uint16_t mark;      /* w12 - number of the mark that created the entity */
    pos_t trigger;      /* b16, w18 - position of trigger box */
    pos_t saved;        /* b1C, w1E*/
    uint16_t sprbase;   /* w20 */
    uint16_t step_no_i; /* w22 */
    uint16_t step_no;   /* w24 */
    int16_t c1;         /* b26 */
    int16_t c2;         /* b28 */
    int16_t offsy;      /* w2C */
    uint8_t ylow;       /* b2A */
    uint8_t latency;    /* b2E */
    ents_flags_t flags;      /* b14 */
    uint8_t prev_n;     /* new */
    pos_t previous;     /* new */
    uint8_t prev_sprite;     /* new */
    bool front;         /* new */
    uint8_t trigsnd;    /* new */
    uint8_t seq;        /* new for rick seq */
    int8_t offsx;       /* new for rick offsx */
} ent_t;

typedef struct
{
    uint8_t w, h;
    uint16_t spr, sni;
    uint8_t trig_w, trig_h;
    uint8_t snd;
} entdata_t;

typedef struct
{
    uint8_t count;
    int8_t dx, dy;
} mvstep_t;

extern ent_t ents_entities[ENT_ENTSNUM + 1];
extern const entdata_t ent_entdata[ENT_NBR_ENTDATA];
extern rect_t *ent_rects;
extern const uint8_t ent_sprseq[ENT_NBR_SPRSEQ];
extern const mvstep_t ent_mvstep[ENT_NBR_MVSTEP];

extern void ent_reset(void);
extern void ent_actvis(uint8_t, uint8_t);
extern void ent_draw(void);
extern void ent_clprev(void);
extern void ent_action(void);

#endif

/* eof */
