/*
 * xrick/src/sysevt.c
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
 * 20021010 SDLK_n replaced by SDLK_Fn because some non-US keyboards
 *          requires that SHIFT be pressed to input numbers.
 */

#include <SDL.h>

#include "system.h"
#include "game.h"

#include "control.h"
#include "sysvid.h"

//#define SYSJOY_RANGE 3280


static SDL_Event event;


/*
 * Process an event
 */
static void
processEvent()
{
    switch (event.type)
    {
    case SDL_KEYDOWN:
        switch (event.key.keysym.sym)
        {
        case SDLK_UP:
            control.up = true;
            break;
        case SDLK_DOWN:
            control.down = true;
            break;
        case SDLK_LEFT:
            control.left = true;
            break;
        case SDLK_RIGHT:
            control.right = true;
            break;
        case SDLK_p:
            control.pause = true;
            break;
        case SDLK_e:
            control.terminate = true;
            break;
        case SDLK_ESCAPE:
            control.exit = true;
            break;
        case SDLK_SPACE:
            control.fire = true;
            break;
        case SDLK_F1:
            sysvid_toggleFullscreen();
            break;
        case SDLK_F2:
            //sysvid_zoom(-1);
            break;
        case SDLK_F3:
            //sysvid_zoom(+1);
            break;
#ifdef ENABLE_SOUND
        case SDLK_F4:
            syssnd_toggleMute();
            break;
        case SDLK_F5:
            syssnd_vol(-1);
            break;
        case SDLK_F6:
            syssnd_vol(+1);
            break;
#endif
        case SDLK_F7:
            game_toggleCheat(CHEAT1);
            break;
        case SDLK_F8:
            game_toggleCheat(CHEAT2);
            break;
        case SDLK_F9:
            game_toggleCheat(CHEAT3);
            break;
        default:
            break;
        }
        break;
    case SDL_KEYUP:
        switch (event.key.keysym.sym)
        {
        case SDLK_UP:
            control.up = false;
            break;
        case SDLK_DOWN:
            control.down = false;
            break;
        case SDLK_LEFT:
            control.left = false;
            break;
        case SDLK_RIGHT:
            control.right = false;
            break;
        case SDLK_p:
            control.pause = false;
            break;
        case SDLK_e:
            control.terminate = false;
            break;
        case SDLK_ESCAPE:
            control.exit = false;
            break;
        case SDLK_SPACE:
            control.fire = false;
            break;
        default:
            break;
        }
        break;
    case SDL_QUIT:
        /* player tries to close the window -- this is the same as pressing ESC */
        control.exit = true;
        break;
#ifdef ENABLE_FOCUS
    case SDL_WINDOWEVENT:
    {
        switch (event.window.event)
        {
        case SDL_WINDOWEVENT_ENTER:
        case SDL_WINDOWEVENT_FOCUS_GAINED:
        case SDL_WINDOWEVENT_RESTORED:
            control_active = true;
            break;
        case SDL_WINDOWEVENT_LEAVE:
        case SDL_WINDOWEVENT_FOCUS_LOST:
        case SDL_WINDOWEVENT_MINIMIZED:
            control_active = false;
            break;
        default:
            break;
        }
    }
    break;
#endif
#ifdef ENABLE_JOYSTICK
    case SDL_JOYAXISMOTION:
        if (event.jaxis.axis == 0)    /* left-right */
        {
            if (event.jaxis.value < -SYSJOY_RANGE)    /* left */
            {
                control.left = true;
                control.right = false;
            }
            else if (event.jaxis.value > SYSJOY_RANGE)    /* right */
            {
                control.right = true;
                control.left = false;
            }
            else    /* center */
            {
                control.left = false;
                control.right = false;
            }
        }
        if (event.jaxis.axis == 1)    /* up-down */
        {
            if (event.jaxis.value < -SYSJOY_RANGE)    /* up */
            {
                control.up = true;
                control.down = false;
            }
            else if (event.jaxis.value > SYSJOY_RANGE)    /* down */
            {
                control.down = true;
                control.up = false;
            }
            else    /* center */
            {
                control.up = false;
                control.down = false;
            }
        }
        break;
    case SDL_JOYBUTTONDOWN:
        control.fire = true;
        break;
    case SDL_JOYBUTTONUP:
        control.fire = false;
        break;
#endif
    default:
        break;
    }
}

/*
 * Process events, if any, then return
 */
void
sysevt_poll(void)
{
    while (SDL_PollEvent(&event))
        processEvent();
}

/*
 * Wait for an event, then process it and return
 */
void
sysevt_wait(void)
{
    SDL_WaitEvent(&event);
    processEvent();
}

/* eof */



