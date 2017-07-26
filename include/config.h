/*
 * xrick/include/config.h
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

#ifndef CONFIG_H
#define CONFIG_H

/* version */
#define VERSION "021212"

/* joystick support */
#undef ENABLE_JOYSTICK

/* sound support */
#define ENABLE_SOUND

/* auto-defocus support */
/* does seem to cause all sorts of problems on BeOS, Windows... */
#undef ENABLE_FOCUS

/* development tools */
#undef DEBUG /* see include/debug.h */

#endif

/* eof */


