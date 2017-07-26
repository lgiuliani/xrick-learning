/*
 * xrick/include/sysvid.h
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

#ifndef SYSVID_H
#define SYSVID_H

#include "rects.h"
#include "img.h"

/*
 * video section
 */
extern void sysvid_init(void);
extern void sysvid_shutdown(void);
extern void sysvid_update(rect_t const *);
extern void sysvid_clear(void);
//extern void sysvid_zoom(int8_t);
extern void sysvid_toggleFullscreen(void);

extern uint8_t *sysvid_fb;  /* frame buffer */

#endif

/* eof */


