/*
 * xrick/src/draw.c
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
 * This is the only file which accesses the video. Anything calling d_*
 * function should be video-independant.
 *
 * draw.c draws into a 320x200 or 0x0140x0xc8 8-bits depth frame buffer,
 * using the CGA 2 bits color codes. It is up to the video to figure out
 * how to display the frame buffer. Whatever draw.c does, does not show
 * until the screen is explicitely refreshed.
 *
 * The "screen" is the whole 0x0140 by 0x00c8 screen, coordinates go from
 * 0x0000,0x0000 to 0x013f,0x00c7.
 *
 * The "map" is a 0x0100 by 0x0140 rectangle that represents the active
 * game area.
 *
 * Relative to the screen, the "map" is located at 0x0020,-0x0040 : the
 * "map" is composed of two hidden 0x0100 by 0x0040 rectangles (one at the
 * top and one at the bottom) and one visible 0x0100 by 0x00c0 rectangle (in
 * the middle).
 *
 * The "map screen" is the visible rectangle ; it is a 0x0100 by 0xc0
 * rectangle located at 0x0020,0x00.
 *
 * Coordinates can be relative to the screen, the map, or the map screen.
 *
 * Coordinates can be expressed in pixels. When relative to the map or the
 * map screen, they can also be expressed in tiles, the map being composed
 * of rows of 0x20 tiles of 0x08 by 0x08 pixels.
 */
//#include <stdint.h>
#include <assert.h>
#include "system.h"
#include "game.h"
#include "draw.h"

#include "sysvid.h"
#include "sprites.h"
#include "tiles.h"
#include "maps.h"


/* Globals */

/* Locals */
/* map coordinates of the screen */
/* map coordinates of the top of the hidden bottom of the map */
static const uint16_t DRAW_XYMAP_HBTOP = 0x0100;
static const int16_t DRAW_XYMAP_SCRLEFT = -0x0020;
static const int16_t DRAW_XYMAP_SCRTOP = 0x0040;

/* counters positions (pixels, screen) */
/*static const uint16_t DRAW_STATUS_BULLETS_X = 0x68;
static const uint16_t DRAW_STATUS_BOMBS_X = 0xA8;
static const uint16_t DRAW_STATUS_SCORE_X = 0x20;
static const uint16_t DRAW_STATUS_LIVES_X = 0xF0;
static const uint16_t  DRAW_STATUS_Y = 0;
*/
enum
{
    DRAW_STATUS_BULLETS_X = 0x68,
    DRAW_STATUS_BOMBS_X = 0xA8,
    DRAW_STATUS_SCORE_X = 0x20,
    DRAW_STATUS_LIVES_X = 0xF0,
    DRAW_STATUS_Y = 0,
};

/* Local prototypes */
static void draw_convertfrom4bpp(uint32_t, uint8_t *);

/*
 * public vars
 */

uint8_t draw_tilesBank; /* tile number offset */
rect_t draw_STATUSRECT =
{
    .x = DRAW_STATUS_SCORE_X,
    .y = DRAW_STATUS_Y,
    .width = DRAW_STATUS_LIVES_X + GAME_LIVES_INIT * 8 - DRAW_STATUS_SCORE_X,
    .height = 8,
    .next = NULL
};
rect_t draw_SCREENRECT = { 0, 0, SYSVID_WIDTH, SYSVID_HEIGHT, NULL };

/*
 * private vars
 */
static uint8_t *fb;     /* frame buffer pointer */


/*
 * Set the frame buffer pointer
 *
 * x, y: position (pixels, screen)
 */
void
draw_setfb(uint16_t x, uint16_t y)
{
    fb = sysvid_fb + x + y * SYSVID_WIDTH;
}


/*
 * Check if not visible in map screen
 *
 * x, y: position (pixels, map)
 * width, height: dimension
 * return: true if fully clipped, false if still (at least partly) visible
 */
bool
draw_isOutsideScreen(pos_t position, dim_t dimension)
{
    if (position.x + dimension.width < 0) return true;
    if (position.x > 0x0100) return true;
    if (position.y + (int16_t)dimension.height < DRAW_XYMAP_SCRTOP) return true;
    if (position.y >= (int16_t)DRAW_XYMAP_HBTOP) return true;

    return false;
}

/*
 * Clip to map screen
 *
 * x, y: position (pixels, map) CHANGED clipped
 * width, height: dimension CHANGED clipped
 */
void draw_clipToMapScreen(pos_t *position, dim_t *dimension)
{
    if (position->x < 0)
    {
        dimension->width += position->x;
        position->x = 0;
    }
    else if (position->x + dimension->width > 0x100)
        dimension->width = 0x100 - position->x;


    if (position->y < DRAW_XYMAP_SCRTOP)
    {
        dimension->height += position->y - DRAW_XYMAP_SCRTOP;
        position->y = DRAW_XYMAP_SCRTOP;
    }
    else if (position->y + dimension->height > DRAW_XYMAP_HBTOP)
        dimension->height = DRAW_XYMAP_HBTOP - position->y;
}


/*
 * Draw a list of tiles onto the frame buffer
 * start at position indicated by fb ; at the end of each (sub)list,
 * perform a "carriage return + line feed" i.e. go back to the initial
 * position then go down one tile row (8 pixels)
 *
 * ASM 1e33
 * fb: CHANGED (see above)
 * draw_tllst: CHANGED points to the element following 0xfe/0xff end code
 */
/*
 * Draw a list of tiles onto the frame buffer -- same as draw_tilesList,
 * but accept an immediate string as parameter. Note that the string needs
 * to be properly terminated with 0xfe (\376) and 0xff (\377) chars.
 */
void
draw_tilesList(uint8_t const *tllst)
{
    uint8_t i;
    uint8_t *t;

    t = fb;
    i = 0;
    while (i != 0xFE)    /* draw sub-list */
    {
        i = *(tllst++);
        while (i != 0xFF && i != 0xFE)    /* while not end */
        {
            draw_tile(i);  /* draw tile */
            i = *(tllst++);
        }

        t += 8 * SYSVID_WIDTH;  /* go down one tile i.e. 8 lines */
        fb = t;
    }
}


/*
 * Draw a sub-list of tiles onto the frame buffer
 * start at position indicated by fb ; leave fb pointing to the next
 * tile to the right of the last tile drawn
 *
 * ASM 1e41
 * fpb: CHANGED (see above)
 * draw_tllst: CHANGED points to the element following 0xfe/0xff end code
 * returns: end code (0xfe : end of list ; 0xff : end of sub-list)
 */
void
draw_tilesSubList(uint8_t const *tllst)
{
    uint8_t i;

    i = *(tllst++);
    while (i != 0xFF && i != 0xFE)    /* while not end */
    {
        draw_tile(i);  /* draw tile */
        i = *(tllst++);
    }
}


/*
 * Draw a tile
 * at position indicated by fb ; leave fb pointing to the next tile
 * to the right of the tile drawn
 *
 * ASM 1e6c
 * tlnbr: tile number
 * draw_filter: CGA colors filter
 * fb: CHANGED (see above)
 */
void
draw_tile(uint8_t tileNumber)
{
    uint8_t *f;
    uint8_t i;
    uint32_t x;

    f = fb;  /* frame buffer */
    for (i = 0; i < 8; i++)    /* for all 8 pixel lines */
    {
        x = tiles_data[draw_tilesBank][tileNumber][i];
        draw_convertfrom4bpp(x, f);
        f += SYSVID_WIDTH;  /* next line */
    }

    fb += 8;  /* next tile */
}


/*
 * Draw a sprite
 *
 * For intro of each level
 */
void
draw_sprite(uint8_t number)
{
    uint8_t *f;
    uint16_t g=0;
    uint32_t d;
    uint_fast8_t i,j;
    uint8_t k;

    for (i = 0; i < 0x15; i++) /* rows */
    {
        f = fb;
        for (j = 0; j < 4; j++) /* cols */
        {
            d = sprites_data[number][g++];
            for (k = 8; k > 0 ; k--)
            {
                if (d & 0x0F)
                    f[k] = (f[k] & 0xF0) | (d & 0x0F);
                d >>= 4;
            }
            f += 8;
        }
        fb += SYSVID_WIDTH;
    }
}


/*
 * Draw a sprite
 *
 * NOTE re-using original ST graphics format
 */
void
draw_sprite2(uint8_t number, pos_t position, dim_t dim0, bool front)
{
    uint32_t d = 0;                 /* sprite data */
    pos_t clipped = position;       /* clipped x, y */
    dim_t dimension = dim0; //{0x20, 0x15}; /* width, height */
    int16_t r, c,                   /* row, column */
            i,
            im;                     /* tile flag shifter */
    map_flags_t flg;                /* tile flag */

    if (draw_isOutsideScreen(clipped, dimension))  /* return if not visible */
        return;
    draw_clipToMapScreen(&clipped, &dimension);

    draw_setfb(clipped.x - DRAW_XYMAP_SCRLEFT, clipped.y - DRAW_XYMAP_SCRTOP + 8);

    assert(dimension.height <= 0x15);
    for (r = 0; r < dimension.height; r++)
    {
        if (position.y + r < clipped.y)
            break;

        im = position.x - (position.x & 0xfff8);
        flg = map_eflg[map_map[(position.y + r) >> 3][(position.x + 0x1f)>> 3]];
        for (i = 3; i >=0; i--)
        {
            d = sprites_data[number][r*4 + i];
            for (c = i*8+7; c >= i*8; c--)
            {
                if (im == 0)
                {
                    flg = map_eflg[map_map[(position.y + r) >> 3][(position.x + c) >> 3]];
                    im = 8;
                }
                im--;

                if ((c < dimension.width && position.x + c >= clipped.x )
                    && (front || game_cheat.xrayvision || !flg.foreground)
                    && ((d & 0x0F) != 0) )
                {
                    fb[c] = (fb[c] & 0xF0) | (d & 0x0F);

                    if (game_cheat.xrayvision)
                        fb[c] |= 0x10;
                }
                d >>= 4;
            }
        }

        fb += SYSVID_WIDTH;
    }
}


/*
 * Redraw the map behind a sprite
 * align to tile column and row, and clip
 *
 * x, y: sprite position (pixels, map).
 */
void
draw_spriteBackground(pos_t position)
{
    dim_t maxi;
    pos_t mapped;
    uint16_t xs, ys;
    uint8_t r,c;

    /* align to column and row, prepare map coordinate, and clip */
    mapped.x = position.x & 0xFFF8;
    mapped.y = position.y & 0xFFF8;
    maxi.width = ((position.x - mapped.x) == 0 ? 0x20 : 0x28);  /* width, 4 tl cols, 8 pix each */
    maxi.height = ((position.y & 0x04) != 0) ? 0x20 : 0x18;  /* height, 3 or 4 tile rows */
    if (draw_isOutsideScreen(mapped, maxi))  /* don't draw if fully clipped */
        return;
    draw_clipToMapScreen(&mapped, &maxi);

    /* get back to screen */
    xs = (uint16_t) (mapped.x - DRAW_XYMAP_SCRLEFT);
    ys = (uint16_t) (mapped.y - DRAW_XYMAP_SCRTOP);

    mapped.x >>= 3;
    mapped.y >>= 3;
    maxi.width >>= 3;
    maxi.height >>= 3;

    /* draw */

    for (r = 0; r < maxi.height; r++)    /* for each row */
    {
        draw_setfb(xs, 8 + ys + (r *8 ));

        for (c = 0; c < maxi.width; c++)    /* for each column */
        {
            draw_tile(map_map[mapped.y + r][mapped.x + c]);
        }
    }
}


/*
 * Draw entire map screen background tiles onto frame buffer.
 *
 * ASM 0af5, 0a54
 */
void
draw_map (void)
{
    uint8_t i,j;
    draw_tilesBank = map_tilesBank;

    for (i = 1; i <= 0x18; i++)    /* 0x18 rows */
    {
        draw_setfb(0x20, i * 8);

        for (j = 0; j < 0x20; j++)  /* 0x20 tiles per row */
            draw_tile(map_map[i+7][j]);
    }
}


/*
 * Draw status indicators
 *
 * ASM 0309
 */
void
draw_drawStatus(void)
{
    /*
     * three special tile numbers
     */
    const uint8_t TILES_BULLET = 0x01;
    const uint8_t TILES_BOMB = 0x02;
    const uint8_t TILES_RICK  = 0x03;

    uint_fast8_t i;
    uint8_t score[] = { 0x30 + (uint8_t)((game_score/100000) % 10),
                        0x30 + (uint8_t)((game_score/10000) % 10),
                        0x30 + (uint8_t)((game_score/1000) % 10),
                        0x30 + (uint8_t)((game_score/100) % 10),
                        0x30 + (uint8_t)((game_score/10) % 10),
                        0x30 + (uint8_t)(game_score % 10),
                        0xfe
                      };

    draw_tilesBank = 0;

    draw_setfb(DRAW_STATUS_SCORE_X, DRAW_STATUS_Y);
    draw_tilesList(score);

    draw_setfb(DRAW_STATUS_BULLETS_X, DRAW_STATUS_Y);
    for (i = 0; i < game_bullets; i++)
        draw_tile(TILES_BULLET);

    draw_setfb(DRAW_STATUS_BOMBS_X, DRAW_STATUS_Y);
    for (i = 0; i < game_bombs; i++)
        draw_tile(TILES_BOMB);

    draw_setfb(DRAW_STATUS_LIVES_X, DRAW_STATUS_Y);
    for (i = 0; i < game_lives; i++)
        draw_tile(TILES_RICK);
}


/*
 * Draw info indicators
 */
void
draw_infos(void)
{
    draw_tilesBank = 0;

    draw_setfb(0x00, DRAW_STATUS_Y);
    draw_tile(game_cheat.unlimited ? 'T' : '@');
    draw_setfb(0x08, DRAW_STATUS_Y);
    draw_tile(game_cheat.immortal ? 'N' : '@');
    draw_setfb(0x10, DRAW_STATUS_Y);
    draw_tile(game_cheat.xrayvision ? 'V' : '@');
}


/*
 * Clear status indicators
 */
void
draw_clearStatus(void)
{
    uint_fast8_t i;

    draw_tilesBank = 0;
    draw_setfb(DRAW_STATUS_SCORE_X, DRAW_STATUS_Y);

    for (i = 0 ; i < ((DRAW_STATUS_LIVES_X - DRAW_STATUS_SCORE_X)/8 + 6); i++)
    {
        draw_tile('@');
    }
}


/*
 * Draw a picture from coord 0,0, to screen width
 */
void
draw_pic(uint16_t height, uint32_t const *pic)
{
    uint16_t i;

    draw_setfb(0, 0);
    for ( i = 0; i < (height * SYSVID_WIDTH / 8); i++)
    {
        draw_convertfrom4bpp(pic[i], fb);
        fb += 8;
    }
}


/*
 * perform the transformation from ST 4 bits
 * per pixel to frame buffer 8 bits per pixels
 */
void
draw_convertfrom4bpp(uint32_t value, uint8_t *result)
{
    uint8_t i;

    for (i=8; i>0; i--)
    {
        result[i] = value & 0x0F;
        value >>= 4;
    }
}

void draw_gettoscreen(pos_t *position)
{
    position->x -= DRAW_XYMAP_SCRLEFT;
    position->y -= (DRAW_XYMAP_SCRTOP - 8);
}

/* eof */
