/*
 * xrick/include/maps.h
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

#ifndef MAPS_H
#define MAPS_H

#include "ents.h"

#define MAP_NBR_MAPS 0x05
#define MAP_NBR_SUBMAPS 0x2F
#define MAP_NBR_CONNECT 0x99
#define MAP_NBR_BNUMS 0x1FD8
#define MAP_NBR_BLOCKS 0x0100
#define MAP_NBR_MARKS 0x020B
#define MAP_NBR_EFLGC 0x0020

/*
 * map row definitions, for three zones : hidden top, screen, hidden bottom
 * the three zones compose map_map, which contains the definition of the
 * current portion of the submap.
 */
#define MAP_ROW_HTTOP 0x00
#define MAP_ROW_HTBOT 0x07
#define MAP_ROW_SCRTOP 0x08
#define MAP_ROW_SCRBOT 0x1F
#define MAP_ROW_HBTOP 0x20
#define MAP_ROW_HBBOT 0x27

extern uint8_t map_map[0x2c][0x20];

/*
 * main maps
 */
typedef struct
{
    pos_t initial_position; /* initial position for rick */
    uint16_t row;		    /* initial map_map top row within the submap */
    uint16_t submap;	    /* initial submap */
    char *tune;	            /* map tune */
} map_t;

extern const map_t map_maps[MAP_NBR_MAPS];

/*
 * sub maps
 */
typedef struct
{
    uint16_t page;            /* tiles page */
    uint16_t bnum;            /* first block number */
    uint16_t connect;         /* first connection */
    uint16_t mark;            /* first entity mark */
} submap_t;

extern const submap_t map_submaps[MAP_NBR_SUBMAPS];

/*
 * connections
 */
typedef struct
{
    const uint8_t dir;  /*  could be dir_t */
    const uint8_t rowout;
    const uint8_t submap;
    const uint8_t rowin;
} connect_t;

extern const connect_t map_connect[MAP_NBR_CONNECT];

/*
 * blocks - one block is 4 by 4 tiles.
 */
typedef uint8_t block_t[0x10];

extern const block_t map_blocks[MAP_NBR_BLOCKS];

/*
 * flags for map_marks[].ent ("yes" when set)
 *
 * MAP_MARK_NACT: this mark is not active anymore.
 */
#define MAP_MARK_NACT (0x80)

/*
 * mark structure
 */
typedef struct
{
    const uint8_t row;
    uint8_t ent;
    const uint8_t flags;
    const uint8_t xy;  /* bits XXXX XYYY (from b03) with X->x, Y->y */
    const uint8_t lt;  /* bits XXXX XNNN (from b04) with X->trig_x, NNN->lat & trig_y */
} mark_t;

extern mark_t map_marks[MAP_NBR_MARKS];

/*
 * block numbers, i.e. array of rows of 8 blocks
 */
extern const uint8_t map_bnums[MAP_NBR_BNUMS];

/*
 * flags for map_eflg[map_map[row][col]]  ("yes" when set)
 *
 * MAP_EFLG_VERT: vertical move only (usually on top of _CLIMB).
 * MAP_EFLG_SOLID: solid block, can't go through.
 * MAP_EFLG_SPAD: super pad. can't go through, but sends entities to the sky.
 * MAP_EFLG_WAYUP: solid block, can't go through except when going up.
 * MAP_EFLG_FGND: foreground (hides entities).
 * MAP_EFLG_LETHAL: lethal (kill entities).
 * MAP_EFLG_CLIMB: entities can climb here.
 * MAP_EFLG_01:
 */
typedef struct
{
    bool once:1;
    bool climb:1;
    bool lethal:1;
    bool foreground:1;
    bool wayup:1;
    bool superpad:1;
    bool solid:1;
    bool vertical:1;
} map_flags_t;


extern const uint8_t map_eflg_c[MAP_NBR_EFLGC];  /* compressed */
extern map_flags_t map_eflg[0x100];  /* current */

/*
 * map_map top row within the submap
 */
extern uint8_t map_frow;

/*
 * tiles offset
 */
extern uint8_t map_tilesBank;

extern void map_expand(void);
extern void map_init(void);
extern bool map_chain(void);
extern void map_resetMarks(void);
extern uint8_t map_find_frow(uint16_t submap);

#endif

/* eof */
