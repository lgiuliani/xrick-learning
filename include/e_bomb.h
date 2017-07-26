/*
 * xrick/include/e_bomb.h
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

#ifndef E_BOMB_H
#define E_BOMB_H


#define E_BOMB_NO 3
#define E_BOMB_ENT ents_entities[E_BOMB_NO]

extern bool e_bomb_isExplosing(uint8_t);
extern void e_bomb_init(pos_t);
extern void e_bomb_action(uint8_t);
extern bool e_bomb_isTriggering(uint8_t);

#endif

/* eof */
