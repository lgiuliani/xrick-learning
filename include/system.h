/*
 * xrick/include/system.h
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

#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>
#include <stdbool.h> /* bool */
#include <stddef.h> /* NULL */

#include "config.h"

/*
 * If compiling w/gcc, then we can use attributes. UNUSED(x) flags a
 * parameter or a variable as potentially being unused, so that gcc doesn't
 * complain about it.
 *
 * Note: from OpenAL code: Darwin OS cc is based on gcc and has __GNUC__
 * defined, yet does not support attributes. So in theory we should exclude
 * Darwin target here.
 */
#ifdef __GNUC__
#define UNUSED(x) x __attribute((unused))
#else
#define UNUSED(x) x
#endif

/*
 * Detect Microsoft Visual C
 */
#ifdef _MSC_VER
#define __MSVC__
/*
 * FIXME disable "integral size mismatch in argument; conversion supplied" warning
 * as long as the code has not been cleared -- there are so many of them...
 */

#pragma warning( disable : 4761 )
#endif

/*
 * Detect Microsoft Windows
 */
#ifdef WIN32
#define __WIN32__
#endif

/* this must be after typedefs because it relies on types defined above */
#include "rects.h"
#include "img.h"

typedef enum { RIGHT = 0, LEFT = 1, NOT_FIND = 0xFF} dir_t;

typedef struct
{
    int16_t x;
    int16_t y;
} pos_t;

typedef struct {
    uint16_t width;
    uint16_t height;
} dim_t;

typedef enum
{
    RUNNING,
    DONE
} state_t;


/*
 * main section
 */
extern void sys_init(int, char **);
extern void sys_shutdown(void);
extern void sys_panic(char const *, ...);
extern void sys_printf(char const *, ...);
extern uint32_t sys_gettime(void);
extern void sys_sleep(uint32_t);

//#define SYSVID_ZOOM 2
#define SYSVID_MAXZOOM 4
enum screen
{
    SYSVID_WIDTH = 320,     /* 0x140 */
    SYSVID_HEIGHT = 200     /* 0x0C8 */
};

/*
 * events section
 */
extern void sysevt_poll(void);
extern void sysevt_wait(void);

/*
 * keyboard section
 */
typedef struct
{
    uint8_t up;
    uint8_t down;
    uint8_t left;
    uint8_t right;
    uint8_t pause;
    uint8_t end;
    uint8_t xtra;
    uint8_t fire;
} syskbd_t;
extern syskbd_t syskbd;

/*
 * sound section
 */
#ifdef ENABLE_SOUND
typedef struct
{
//#ifdef DEBUG
    char *name;
//#endif
    uint8_t *buf;
    uint32_t len;
    bool dispose;
} sound_t;

extern void syssnd_init(void);
extern void syssnd_shutdown(void);
extern void syssnd_vol(int8_t);
extern void syssnd_toggleMute(void);
extern void syssnd_play(sound_t *, int8_t);
extern void syssnd_pause(bool, bool);
extern void syssnd_stopsound(sound_t const *);
extern void syssnd_stopall(void);
extern sound_t *syssnd_load(char const *name);
extern void syssnd_free(sound_t *);
#endif

/*
 * args section
 */
typedef struct {
    int period;
    int map;
    int submap;
    int zoom;
    bool fullscreen;
#ifdef ENABLE_SOUND
    bool nosound;
    int vol;
#endif
    char *data;
} sysarg_t;
extern sysarg_t sysarg_args;

extern void sysarg_init(int, char **);

/*
 * joystick section
 */
#ifdef ENABLE_JOYSTICK
extern void sysjoy_init(void);
extern void sysjoy_shutdown(void);
#endif


#endif

/* eof */


