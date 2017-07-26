/*
 * xrick/src/syssnd.c
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

#include <SDL.h>
#include "config.h"

#ifdef ENABLE_SOUND

#include "system.h"
#include "game.h"
#include "syssnd.h"
#include "debug.h"

#define ADJVOL(S) (((S)*sndVol)/SDL_MIX_MAXVOLUME)

static uint8_t isAudioActive = false;
static channel_t channel[SYSSND_MIXCHANNELS];

static uint8_t sndVol = SDL_MIX_MAXVOLUME;  /* internal volume */
static uint8_t sndUVol = SYSSND_MAXVOL;  /* user-selected volume */
static bool sndMute = false;  /* mute flag */

static SDL_mutex *sndlock;

/*
 * prototypes
 */
static bool sdlRWops_open(SDL_RWops *context, char const *name);
static Sint64 sdlRWops_seek(SDL_RWops *context, Sint64 offset, int whence);
static size_t sdlRWops_read(SDL_RWops *context, void *ptr, size_t size, size_t maxnum);
/* static size_t sdlRWops_write(SDL_RWops *context, const void *ptr, int size, int num);*/
static int sdlRWops_close(SDL_RWops *context);
static void end_channel(uint8_t);

/*
 * Callback -- this is also where all sound mixing is done
 *
 * Note: it may not be that much a good idea to do all the mixing here ; it
 * may be more efficient to mix samples every frame, or maybe everytime a
 * new sound is sent to be played. I don't know.
 */
static void syssnd_callback(UNUSED(void *userdata), uint8_t *stream, int len)
{
    uint8_t c;
    int16_t s;
    uint32_t i;

    SDL_mutexP(sndlock);

    for (i = 0; i < (uint32_t)len; i++)
    {
        s = 0;
        for (c = 0; c < SYSSND_MIXCHANNELS; c++)
        {
            if (channel[c].loop != 0)    /* channel is active */
            {
                if (channel[c].len > 0)    /* not ending */
                {
                    s += ADJVOL(*channel[c].buf - 0x80);
                    channel[c].buf++;
                    channel[c].len--;
                }
                else    /* ending */
                {
                    if (channel[c].loop > 0) channel[c].loop--;
                    if (channel[c].loop)    /* just loop */
                    {
                        IFDEBUG_AUDIO2(sys_printf("xrick/audio: channel %d - loop\n", c););
                        channel[c].buf = channel[c].snd->buf;
                        channel[c].len = channel[c].snd->len;
                        s += ADJVOL(*channel[c].buf - 0x80);
                        channel[c].buf++;
                        channel[c].len--;
                    }
                    else    /* end for real */
                    {
                        IFDEBUG_AUDIO2(sys_printf("xrick/audio: channel %d - end\n", c););
                        end_channel(c);
                    }
                }
            }
        }
        if (sndMute)
            stream[i] = 0x80;
        else
        {
            s += 0x80;
            if (s > 0xff) s = 0xff;
            if (s < 0x00) s = 0x00;
            stream[i] = (uint8_t)s;
        }
    }

    memcpy(stream, stream, (size_t)len);

    SDL_mutexV(sndlock);
}

static void
end_channel(uint8_t c)
{
    channel[c].loop = 0;
    if (channel[c].snd->dispose)
        syssnd_free(channel[c].snd);
    channel[c].snd = NULL;
}

void
syssnd_init(void)
{
    SDL_AudioSpec desired, obtained;
    uint16_t c;

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
    {
        IFDEBUG_AUDIO(
            sys_printf("xrick/audio: can not initialize audio subsystem\n");
        );
        return;
    }

    desired.freq = SYSSND_FREQ;
    desired.format = AUDIO_U8;
    desired.channels = SYSSND_CHANNELS;
    desired.samples = SYSSND_MIXSAMPLES;
    desired.callback = syssnd_callback;
    desired.userdata = NULL;

    if (SDL_OpenAudio(&desired, &obtained) < 0)
    {
        IFDEBUG_AUDIO(
            sys_printf("xrick/audio: can not open audio (%s)\n", SDL_GetError());
        );
        return;
    }

    sndlock = SDL_CreateMutex();
    if (sndlock == NULL)
    {
        IFDEBUG_AUDIO(sys_printf("xrick/audio: can not create lock\n"););
        SDL_CloseAudio();
        return;
    }

    if (sysarg_args.vol != 0)
    {
        sndUVol = (uint8_t) sysarg_args.vol;
        sndVol = (uint8_t) (SDL_MIX_MAXVOLUME * sndUVol / SYSSND_MAXVOL);
    }

    for (c = 0; c < SYSSND_MIXCHANNELS; c++)
        channel[c].loop = 0;  /* deactivate */

    isAudioActive = true;
    SDL_PauseAudio(0);
}

/*
 * Shutdown
 */
void
syssnd_shutdown(void)
{
    if (!isAudioActive) return;

    SDL_CloseAudio();
    SDL_DestroyMutex(sndlock);
    isAudioActive = false;
}

/*
 * Toggle mute
 *
 * When muted, sounds are still managed but not sent to the dsp, hence
 * it is possible to un-mute at any time.
 */
void
syssnd_toggleMute(void)
{
    SDL_mutexP(sndlock);
    sndMute = !sndMute;
    SDL_mutexV(sndlock);
}

void
syssnd_vol(int8_t d)
{
    if ((d < 0 && sndUVol > 0) ||
            (d > 0 && sndUVol < SYSSND_MAXVOL))
    {
        sndUVol += d;
        SDL_mutexP(sndlock);
        sndVol = (uint8_t) (SDL_MIX_MAXVOLUME * sndUVol / SYSSND_MAXVOL);
        SDL_mutexV(sndlock);
    }
}

/*
 * Play a sound
 *
 * loop: number of times the sound should be played, -1 to loop forever
 * returns: channel number, or -1 if none was available
 *
 * NOTE if sound is already playing, simply reset it (i.e. can not have
 * twice the same sound playing -- tends to become noisy when too many
 * bad guys die at the same time).
 */
void
syssnd_play(sound_t *sound, int8_t loop)
{
    int8_t c;

    if (!isAudioActive) return;
    if (sound == NULL) return;

    c = 0;
    SDL_mutexP(sndlock);
    while ((channel[c].snd != sound || channel[c].loop == 0) &&
            channel[c].loop != 0 &&
            c < SYSSND_MIXCHANNELS)
        c++;
    if (c == SYSSND_MIXCHANNELS)
        c = -1;

    IFDEBUG_AUDIO(
        if (channel[c].snd == sound && channel[c].loop != 0)
        sys_printf("xrick/sound: already playing %s on channel %d - resetting\n",
                   sound->name, c);
        else if (c >= 0)
            sys_printf("xrick/sound: playing %s on channel %d\n", sound->name, c);
        );

    if (c >= 0)
    {
        channel[c].loop = loop;
        channel[c].snd = sound;
        channel[c].buf = sound->buf;
        channel[c].len = sound->len;
    }
    SDL_mutexV(sndlock);

    return;
}

/*
 * Pause
 *
 * pause: TRUE or FALSE
 * clear: TRUE to cleanup all sounds and make sure we start from scratch
 */
void
syssnd_pause(bool pause, bool clear)
{
    if (!isAudioActive) return;

    if (clear)
    {
        uint8_t c;
        SDL_mutexP(sndlock);
        for (c = 0; c < SYSSND_MIXCHANNELS; c++)
            channel[c].loop = 0;
        SDL_mutexV(sndlock);
    }

    if (pause)
        SDL_PauseAudio(1);
    else
        SDL_PauseAudio(0);
}

/*
 * Stop a sound
 */
void
syssnd_stopsound(sound_t const *sound)
{
    uint8_t i;

    if (!sound) return;

    SDL_mutexP(sndlock);
    for (i = 0; i < SYSSND_MIXCHANNELS; i++)
        if (channel[i].snd == sound) end_channel(i);
    SDL_mutexV(sndlock);
}

/*
 * Stops all channels.
 */
void
syssnd_stopall(void)
{
    uint8_t i;

    SDL_mutexP(sndlock);
    for (i = 0; i < SYSSND_MIXCHANNELS; i++)
        if (channel[i].snd) end_channel(i);
    SDL_mutexV(sndlock);
}

/*
 * Load a sound.
 */
sound_t *
syssnd_load(char const *name)
{
    sound_t *s;
    SDL_RWops *context;
    SDL_AudioSpec audiospec;

    /* alloc context */
    context = malloc(sizeof(SDL_RWops));
    context->seek = sdlRWops_seek;
    context->read = sdlRWops_read;
    /*context->write = sdlRWops_write;*/
    context->close = sdlRWops_close;

    /* open */
    if (!sdlRWops_open(context, name))
    {
        free(context);
        return NULL;
    }

    /* alloc sound */
    s = malloc(sizeof(sound_t));
#ifdef DEBUG
    s->name = malloc(strlen(name) + 1);
    strncpy(s->name, name, strlen(name) + 1);
#endif

    /* read */
    /* second param == 1 -> close source once read */
    if (!SDL_LoadWAV_RW(context, 1, &audiospec, &(s->buf), &(s->len)))
    {
        free(s);
        return NULL;
    }

    s->dispose = false;

    return s;
}

/*
 *
 */
void
syssnd_free(sound_t *s)
{
    if (!s) return;
    if (s->buf) SDL_FreeWAV(s->buf);
    s->buf = NULL;
    s->len = 0;
    free(s);
}

/*
 *
 */
static inline bool
sdlRWops_open(SDL_RWops *context, char const *name)
{
    data_file_t *f;

    f = data_file_open(name);
    if (!f)
        return false;
    context->hidden.unknown.data1 = (void *)f;

    return true;
}

static Sint64 /* Sint64 */
sdlRWops_seek(SDL_RWops * context, Sint64 offset, int whence)
{
    return (Sint64)data_file_seek((data_file_t *)(context->hidden.unknown.data1), offset, whence);
}

static size_t /* size_t */
sdlRWops_read(SDL_RWops * context, void *ptr, size_t size, size_t maxnum)
{
    return (size_t)data_file_read((data_file_t *)(context->hidden.unknown.data1), ptr, size, maxnum);
}

/*
static size_t
sdlRWops_write(SDL_RWops *context, const void *ptr, int size, int num)
{
    not implemented
    return (size_t)-1;
}
*/

static int
sdlRWops_close(SDL_RWops *context)
{
    if (context)
    {
        data_file_close((data_file_t *)(context->hidden.unknown.data1));
        free(context);
    }
    return 0;
}

#endif /* ENABLE_SOUND */

/* eof */

