/*
 * xrick/src/sysarg.c
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
 * 20021010 added test to prevent buffer overrun in -keys parsing.
 */

#include <stdlib.h>  /* atoi */
#include <string.h>  /* strcasecmp */

#include <SDL.h>

#include "system.h"
#include "game.h"

#include "maps.h"
#include "syssnd.h"

/* handle Microsoft Visual C (must come after system.h!) */
#ifdef __MSVC__
#define strcasecmp stricmp
#endif

sysarg_t sysarg_args = {
    .period = 0,
    .map = 0,
    .submap = 0,
    .fullscreen = false,
    .zoom = 0,
#ifdef ENABLE_SOUND
    .nosound = false,
    .vol = 0,
#endif // ENABLE_SOUND
    .data = NULL
};


/*
 * Fail
 */
static void
sysarg_fail(char const *msg)
{
#ifdef ENABLE_SOUND
    printf("xrick [version #%s]: %s\nusage: xrick [<options>]\n<option> =\n  -h, -help : Display this information. -fullscreen : Run in fullscreen mode.\n    The default is to run in a window.\n  -speed <speed> : Run at speed <speed>. Speed must be an integer between 1\n    (fast) and 100 (slow). The default is %d\n  -zoom <zoom> : Display with zoom factor <zoom>. <zoom> must be an integer\n   between 1 (320x200) and %d (%d times bigger). The default is 2.\n  -map <map> : Start at map number <map>. <map> must be an integer between\n    1 and %d. The default is to start at map number 1\n  -submap <submap> : Start at submap <submap>. <submap> must be an integer\n    between 1 and %d. The default is to start at submap number 1 or, if a map\n    was specified, at the first submap of that map.\n  -keys <left>-<right>-<up>-<down>-<fire> : Override the default key\n    bindings (cf. KeyCodes)\n  -nosound : Disable sounds. The default is to play with sounds enabled.\n  -vol <vol> : Play sounds at volume <vol>. <vol> must be an integer\n    between 0 (silence) and %d (max). The default is to play sounds\n    at maximal volume (%d).\n", VERSION, msg, GAME_PERIOD, SYSVID_MAXZOOM, SYSVID_MAXZOOM, MAP_NBR_MAPS-1, MAP_NBR_SUBMAPS, SYSSND_MAXVOL, SYSSND_MAXVOL);
#else
    printf("xrick [version #%s]: %s\nusage: xrick [<options>]\n<option> =\n  -h, -help : Display this information. -fullscreen : Run in fullscreen mode.\n    The default is to run in a window.\n  -speed <speed> : Run at speed <speed>. Speed must be an integer between 1\n    (fast) and 100 (slow). The default is %d\n  -zoom <zoom> : Display with zoom factor <zoom>. <zoom> must be an integer\n   between 1 (320x200) and %d (%d times bigger). The default is 2.\n  -map <map> : Start at map number <map>. <map> must be an integer between\n    1 and %d. The default is to start at map number 1\n  -submap <submap> : Start at submap <submap>. <submap> must be an integer\n    between 1 and %d. The default is to start at submap number 1 or, if a map\n    was specified, at the first submap of that map.\n  -keys <left>-<right>-<up>-<down>-<fire> : Override the default key\n    bindings (cf. KeyCodes)\n", VERSION, msg, GAME_PERIOD, SYSVID_MAXZOOM, SYSVID_MAXZOOM, MAP_NBR_MAPS-1, MAP_NBR_SUBMAPS);
#endif
    exit(1);
}

/*
 * Read and process arguments
 */
void
sysarg_init(int argc, char **argv)
{
    int i;

    for (i = 1; i < argc; i++)
    {

        if (!strcmp(argv[i], "-fullscreen"))
            sysarg_args.fullscreen = true;

        else if (!strcmp(argv[i], "-help") || !strcmp(argv[i], "-h"))
            sysarg_fail("help");

        else if (!strcmp(argv[i], "-speed"))
        {
            if (++i == argc) sysarg_fail("missing speed value");
            sysarg_args.period = atoi(argv[i]) - 1;
            if (sysarg_args.period < 0 || sysarg_args.period > 99)
                sysarg_fail("invalid speed value");
        }

        /*else if (!strcmp(argv[i], "-keys"))
        {
            if (++i == argc) sysarg_fail("missing key codes");
            if (sysarg_scankeys(argv[i]) == -1)
                sysarg_fail("invalid key codes");
        }*/

        else if (!strcmp(argv[i], "-zoom"))
        {
            if (++i == argc) sysarg_fail("missing zoom value");
            sysarg_args.zoom = atoi(argv[i]);
            if (sysarg_args.zoom < 1 || sysarg_args.zoom > SYSVID_MAXZOOM)
                sysarg_fail("invalid zoom value");
        }

        else if (!strcmp(argv[i], "-map"))
        {
            if (++i == argc) sysarg_fail("missing map number");
            sysarg_args.map = atoi(argv[i]) - 1;
            if (sysarg_args.map < 0 || sysarg_args.map >= MAP_NBR_MAPS-1)
                sysarg_fail("invalid map number");
        }

        else if (!strcmp(argv[i], "-submap"))
        {
            if (++i == argc) sysarg_fail("missing submap number");
            sysarg_args.submap = atoi(argv[i]) - 1;
            if (sysarg_args.submap < 0 || sysarg_args.submap >= MAP_NBR_SUBMAPS)
                sysarg_fail("invalid submap number");
        }
#ifdef ENABLE_SOUND
        else if (!strcmp(argv[i], "-vol"))
        {
            if (++i == argc) sysarg_fail("missing volume");
            sysarg_args.vol = atoi(argv[i]) - 1;
            if (sysarg_args.vol < 0 || sysarg_args.vol >= SYSSND_MAXVOL)
                sysarg_fail("invalid volume");
        }

        else if (!strcmp(argv[i], "-nosound"))
        {
            sysarg_args.nosound = true;
        }
#endif
        else if (!strcmp(argv[i], "-data"))
        {
            i++;
            if (i == argc)
                sysarg_fail("missing data");
            sysarg_args.data = argv[i];
        }

        else
        {
            sysarg_fail("invalid argument(s)");
        }

    }

    /* this is dirty (sort of) */
    if (sysarg_args.submap < 0)
        sysarg_fail("invalid argument(s)");

    if (sysarg_args.submap < 9)
        sysarg_args.map = 0;
    else if (sysarg_args.submap < 20)
        sysarg_args.map = 1;
    else if (sysarg_args.submap < 38)
        sysarg_args.map = 2;
    else
        sysarg_args.map = 3;

    if (sysarg_args.submap == 9 || sysarg_args.submap == 20 ||
            sysarg_args.submap == 38)
        sysarg_args.submap = 0;
}

/* eof */
