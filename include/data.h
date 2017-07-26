/*
 * xrick/include/data.h
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

#ifndef DATA_H
#define DATA_H

#include "system.h"

typedef void *data_file_t;

extern void data_setpath(char const *);
extern void data_closepath(void);

extern data_file_t *data_file_open(char const *);
extern int data_file_seek(data_file_t *file, long offset, int origin);
//extern int data_file_tell(data_file_t *file);
//extern int data_file_size(data_file_t *file);
extern size_t data_file_read(data_file_t *, void *, size_t, size_t);
extern void data_file_close(data_file_t *);

#endif

/* eof */
