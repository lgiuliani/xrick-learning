/*
 * xrick/src/maps.c
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

/*
 * NOTES
 *
 * A map is composed of submaps, which in turn are composed of rows of
 * 0x20 tiles. map_map contains the tiles for the current portion of the
 * current submap, i.e. a little bit more than what appear on the screen,
 * but not the whole submap.
 *
 * map_frow is map_map top row within the submap.
 *
 * Submaps are stored as arrays of blocks, each block being a 4x4 tile
 * array. map_submaps[].bnum points to the first block of the array.
 *
 * Before a submap can be played, it needs to be expanded from blocks
 * to map_map.
 */
#include <assert.h>

#include "system.h"
#include "game.h"
#include "e_rick.h"
#include "maps.h"

#include "e_sbonus.h"

/*
 * global vars
 */
uint8_t map_map[0x2C][0x20];
//uint8_t map_eflg[0x100];
map_flags_t map_eflg[0x100];
uint8_t map_frow;
uint8_t map_tilesBank;


/*
 * prototypes
 */
static void map_eflg_expand(uint8_t);


/*
 * Fill in map_map with tile numbers by expanding blocks.
 *
 * add map_submaps[].bnum to map_frow to find out where to start from.
 * We need to /4 map_frow to convert from tile rows to block rows, then
 * we need to *8 to convert from block rows to block numbers (there
 * are 8 blocks per block row). This is achieved by *2 then &0xfff8.
 */
void
map_expand(void)
{
    uint8_t i, j, k, l;
    uint8_t row, col;
    uint16_t pbnum;

    pbnum = map_submaps[game_submap].bnum + ((2 * map_frow) & 0xfff8);
    row = col = 0;

    for (i = 0; i < 0x0b; i++)    /* 0x0b rows of blocks */
    {
        for (j = 0; j < 0x08; j++)    /* 0x08 blocks per row */
        {
            for (k = 0, l = 0; k < 0x04; k++)    /* expand one block */
            {
                map_map[row+k][col] = map_blocks[map_bnums[pbnum]][l++];
                map_map[row+k][col+1] = map_blocks[map_bnums[pbnum]][l++];
                map_map[row+k][col+2] = map_blocks[map_bnums[pbnum]][l++];
                map_map[row+k][col+3]   = map_blocks[map_bnums[pbnum]][l++];
                //row += 1;
                //col -= 3;
            }
            //row -= 4;
            col += 4;
            pbnum++;
        }
        row += 4;
        col = 0;
    }
}


/*
 * Initialize a new submap
 *
 * ASM 0cc3
 */
void
map_init(void)
{
    map_tilesBank = (map_submaps[game_submap].page == 1) ? 2 : 1;
    map_eflg_expand((map_submaps[game_submap].page == 1) ? 0x10 : 0x00);
    map_expand();
    ent_reset();
    ent_actvis(map_frow + MAP_ROW_SCRTOP, map_frow + MAP_ROW_SCRBOT);
    ent_actvis(map_frow + MAP_ROW_HTTOP, map_frow + MAP_ROW_HTBOT);
    ent_actvis(map_frow + MAP_ROW_HBTOP, map_frow + MAP_ROW_HBBOT);
}


/*
 * Expand entity flags for this map
 *
 * ASM 1117
 */
void
map_eflg_expand(uint8_t offs)
{
    uint8_t i, j, k;

    for (i = 0, k = 0; i < 0x10; i++)
    {
        j = map_eflg_c[offs + i++];
        while (j--)
        {
            map_eflg[k] = (map_flags_t) {
                .once       = map_eflg_c[offs + i] & (1u << 0),
                .climb      = map_eflg_c[offs + i] & (1u << 1),
                .lethal     = map_eflg_c[offs + i] & (1u << 2),
                .foreground = map_eflg_c[offs + i] & (1u << 3),
                .wayup      = map_eflg_c[offs + i] & (1u << 4),
                .superpad   = map_eflg_c[offs + i] & (1u << 5),
                .solid      = map_eflg_c[offs + i] & (1u << 6),
                .vertical   = map_eflg_c[offs + i] & (1u << 7)
            };
            k++;
        }
    }
}


/*
 * Chain (sub)maps
 *
 * ASM 0c08
 * return: TRUE/next submap OK, FALSE/map finished
 */
bool
map_chain(void)
{
    uint16_t c;
    int16_t t;

    game_chsm = false;
    e_sbonus_counting = false;

    /*
     * look for the first connector with compatible row number. if none
     * found, then panic
     */
    for (c = map_submaps[game_submap].connect; ; c++)
    {
        assert(map_connect[c].dir != NOT_FIND);

        if (!e_rick_isConnect(c))
            continue;
        t = ((E_RICK_ENT.position.y >> 3) + map_frow - map_connect[c].rowout);
        if (t < 3)
            break;
    }

    if (map_connect[c].submap == 0xff)
    {
        /* no next submap - request next map */
        return false;
    }

    /* next submap */
    map_frow = map_frow - map_connect[c].rowout + map_connect[c].rowin;
    game_submap = map_connect[c].submap;
    return true;
}


/*
 * Reset all marks, i.e. make them all active again.
 *
 * ASM 0025
 *
 */
void
map_resetMarks(void)
{
    uint16_t i;
    for (i = 0; i < MAP_NBR_MARKS; i++)
        map_marks[i].ent &= ~MAP_MARK_NACT;
}


/*
 * Dirty hack to determine frow from submap
 *
 */
uint8_t
map_find_frow(uint16_t submap)
{
    uint8_t i = 0;

    /* dirty hack to determine frow */
    //while (i < MAP_NBR_CONNECT &&
    //       (map_connect[i].submap != submap || map_connect[i].dir != RIGHT))
    //    i++;
    for (i = 0; i < MAP_NBR_CONNECT; i++)
        if (map_connect[i].submap != submap || map_connect[i].dir != RIGHT)
            break;

    return (map_connect[i].rowin - 0x10);
}

/* eof */
