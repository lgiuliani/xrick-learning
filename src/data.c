/*
 * xrick/src/data.c
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

#include <stdlib.h>  /* malloc */
#include <string.h>
#include <stdio.h>

#include <minizip/unzip.h>

#include "system.h"
#include "data.h"


/*
 * Private typedefs
 */
typedef struct
{
    char *name;
    unzFile zip;
} zipped_t;

/*typedef struct
{
    char *name;
    unzFile zip;
} path_t;*/


/*
 * Static variables
 */
static zipped_t path;

/*
 * Prototypes
 */
static bool str_zipext(char const *);
static char *str_dup(char const *);
static void str_slash(char *);

/*
 *
 */
void
data_setpath(char const *name)
{
    unzFile zip;
    char *n;

    if (str_zipext(name))
    {
        /* path has .zip extension */
        n = str_dup(name);
        str_slash(n);
        zip = unzOpen(n);
        if (!zip)
        {
            free(n);
            sys_panic("(data) can not open data");
        }
        else
        {
            path.zip = zip;
            path.name = n;
        }
    }
    else
    {
        /* path has no .zip extension. it should be a directory */
        /* FIXME check that it is a valid directory */
        path.zip = NULL;
        path.name = str_dup(name);
    }
}

/*
 *
 */
void
data_closepath(void)
{
    if (path.zip)
    {
        unzClose(path.zip);
        path.zip = NULL;
    }
    free(path.name);
    path.name = NULL;
}

/*
 * Open a data file.
 */
data_file_t *
data_file_open(char const *name)
{
    if (path.zip)
    {
        zipped_t *z;
        unzFile zip;

        z = (zipped_t*)malloc(sizeof(zipped_t));
        z->name = str_dup(name);
        zip = unzOpen(path.name);
        z->zip = zip;
        //memcpy(path.zip, z->zip, sizeof(path.zip));
        //z->zip = &path.zip;//unzDup(path.zip);
        if (unzLocateFile(z->zip, name, 0) != UNZ_OK ||
                unzOpenCurrentFile(z->zip) != UNZ_OK)
        {
            unzClose(z->zip);
            free(z);
            z = NULL;
        }
        return (data_file_t *)z;
    }
    else
    {
        char *n;
        FILE *fh;

        n = malloc(strlen(path.name) + strlen(name) + 2);
        sprintf(n, "%s/%s", path.name, name);
        str_slash(n);
        fh = fopen(n, "rb");
        free(n);
        return (data_file_t *)fh;
    }
}

/*
 * Seek.
 */
int
data_file_seek(data_file_t *file, long offset, int origin)
{
    if (path.zip)
    {
        /* not implemented */
        return -1;
    }
    else
    {
        return fseek((FILE *)file, offset, origin);
    }
}

/*
 * Read a file within a data archive.
 */
size_t
data_file_read(data_file_t *file, void *buf, size_t size, size_t count)
{
    if (path.zip)
    {
        return unzReadCurrentFile(((zipped_t *)file)->zip, buf, (unsigned int) (size * count)) / size;
    }
    else
    {
        return fread(buf, size, count, (FILE *)file);
    }
}

/*
 * Close a file within a data archive.
 */
void
data_file_close(data_file_t *file)
{
    if (path.zip)
    {
        unzClose(((zipped_t *)file)->zip);
        ((zipped_t *)file)->zip = NULL;
        free(((zipped_t *)file)->name);
        ((zipped_t *)file)->name = NULL;
    }
    else
    {
        fclose((FILE *)file);
    }
}

/*
 * Returns true if filename has .zip extension.
 */
static bool
str_zipext(char const *name)
{
    size_t len;

    len = strlen(name);
    /* At least 1 char + ".zip" */
    if (len < 5) return false;
    len--;
    if (name[len] != 'p' && name[len] != 'P') return false;
    len--;
    if (name[len] != 'i' && name[len] != 'I') return false;
    len--;
    if (name[len] != 'z' && name[len] != 'Z') return false;
    len--;
    if (name[len] != '.') return false;

    return true;
}

/*
 *
 */
static char *
str_dup(char const *s)
{
    char *pc = NULL;

    if (s != NULL)
    {
        pc = malloc((strlen(s) + 1) * sizeof *pc);
        if (pc != NULL)
            strcpy(pc, s);
    }
    return pc;
}

static inline void
str_slash(char *s)
{
#ifdef __WIN32__
    int i, l;

    l = strlen(s);
    for (i = 0; i < l; i++)
        if (s[i] == '/') s[i] = '\\';
#else
    (void)s;
#endif
    return;
}

/* eof */
