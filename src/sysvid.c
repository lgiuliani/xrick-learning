/*
 * xrick/src/sysvid.c
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
#include <stdlib.h> /* malloc */

#include <SDL.h>

#include "system.h"
#include "sysvid.h"

#ifdef __MSVC__
#include <memory.h> /* memset */
#endif

uint8_t *sysvid_fb; /* frame buffer */

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;

static uint32_t videoFlags;

#include "img_icon.e"

/*
 * Initialise video
 */
void
sysvid_init(void)
{
    SDL_Surface *s_icon; /* for icon */
    uint8_t zoom;

    /* Zoom */
    zoom = (uint8_t) ((sysarg_args.zoom) ? sysarg_args.zoom : 1);

    /* SDL */
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) < 0)
        sys_panic("xrick/video: could not init SDL\n");

    /* various WM stuff */
    SDL_ShowCursor(SDL_DISABLE);

    s_icon = SDL_CreateRGBSurfaceFrom(IMG_ICON->pixels,
                                      IMG_ICON->w,
                                      IMG_ICON->h,
                                      8,
                                      IMG_ICON->w,
                                      0xffffffff, 0xffffffff,
                                      0xffffffff, 0xffffffff);
    SDL_SetWindowIcon(window, s_icon);
    SDL_FreeSurface(s_icon);

    /* video modes and screen */
    videoFlags = SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE;

    if (sysarg_args.fullscreen)
        videoFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    window = SDL_CreateWindow("xrick",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              SYSVID_WIDTH*zoom, SYSVID_HEIGHT*zoom,
                              videoFlags);

    if (window == NULL )
        sys_panic("xrick/video: could not init window\n");

    /* Set Hint before Renderer */
    SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "1");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    /* Create Renderer */
    renderer = SDL_CreateRenderer(window,
                                  -1,
                                  SDL_RENDERER_ACCELERATED|
                                  SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL )
        sys_panic("xrick/video: could not init renderer\n");

    /* This will make sure that the aspect ratio is maintained */
    SDL_RenderSetLogicalSize(renderer, SYSVID_WIDTH, SYSVID_HEIGHT);


    texture = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                SYSVID_WIDTH, SYSVID_HEIGHT);

    /*
     * create video frame buffer
     */
    sysvid_fb = (uint8_t *) malloc(SYSVID_WIDTH * SYSVID_HEIGHT*sizeof(uint8_t));
    if (!sysvid_fb)
        sys_panic("xrick/video: sysvid_fb malloc failed\n");
}

/*
 * Shutdown video
 */
void
sysvid_shutdown(void)
{
    free(sysvid_fb);
    sysvid_fb = NULL;

    SDL_DestroyRenderer(renderer);
    renderer = NULL;

    SDL_DestroyWindow(window);

    SDL_Quit();
}

/*
 * Update screen
 * Convert 8 bits frame buffer+ palette to 32bits ARGB frame buffer then render texture
 */
void
sysvid_update(rect_t const *rects)
{
    /* color table in ARGB format (ALPHA is always to 00) */
    static const uint32_t palette[] = { 0xff000000, 0xffd80000, 0xffb06c68, 0xfff89068,
                                        0xff202420, 0xff0048b0, 0xff006cd8, 0xff204800,
                                        0xff486c20, 0xff482400, 0xff904800, 0xffd86c00,
                                        0xff484848, 0xff686c68, 0xff909090, 0xffb0b4b0
                                      };

    uint8_t *src;
    uint32_t *dest;
    uint32_t *toto;
    int pitch;
    uint_fast16_t y, x;

    if (rects == NULL)
        return;

    SDL_LockTexture(texture, NULL, (void**)&dest, &pitch);
    do
    {
        src = sysvid_fb + rects->x + rects->y * SYSVID_WIDTH;
        toto = dest + rects->x + rects->y * SYSVID_WIDTH;

        for (y = 0; y < rects->height; y++)
        {
            for (x = 0; x < rects->width; x++)
                toto[x] = palette[src[x]];

            toto += SYSVID_WIDTH;
            src += SYSVID_WIDTH;
        }

        rects = rects->next;
    } while (rects);
    SDL_UnlockTexture(texture);

    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}


/*
 * Clear screen
 * (077C)
 */
void
sysvid_clear(void)
{
    memset(sysvid_fb, 0, SYSVID_WIDTH * SYSVID_HEIGHT);
}


/*
 * Toggle fullscreen
 */
void
sysvid_toggleFullscreen(void)
{
    if (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP) /* go window */
        SDL_SetWindowFullscreen(window, SDL_FALSE);
    else                                                    /* go fullscreen */
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
}

/* eof */
