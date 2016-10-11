/*
** brec/sndfunc.c (C) David Monro 1996
**
** Copyright under the GPL - see the file COPYING in this directory
**
**
*/

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>

#define AUDIO "/dev/dsp"

/* Globals */
int audio, abuf_size, fmt_mask;
char *audev = AUDIO;

/* Prototypes */
void sync_audio(void);
void cleanup_audio(void);

/* Extern globals */
extern char *progname;
extern int debug;

/* Extern prototypes */
extern void ErrDie(char *err);
extern void Die(char *err);

void init_sound(int recorder)
{

    /* Attempt to open the audio device */
    audio = open(audev, (recorder)? O_RDONLY : O_WRONLY);
    if (audio == -1)
	ErrDie(audev);
#if 1
    if (ioctl(audio, SNDCTL_DSP_GETBLKSIZE, &abuf_size) < 0) ErrDie(audev);
    if (debug) 
        fprintf(stderr, "abuf_size = %d\n", abuf_size);
#if 0
    if (abuf_size < 1024 || abuf_size > 65536) Die("invalid audio buffer size");
#else
    if (abuf_size < 4096) abuf_size = 4096; /* Seems reasonable */
#endif
#else
    abuf_size = 65536;
#endif
#if 1
    if (ioctl(audio, SNDCTL_DSP_GETFMTS, &fmt_mask) < 0) ErrDie(audev);
#endif
}

void snd_parm(int speed, int bits, int stereo)
{
    static int oldspeed = -1, oldbits = -1, oldstereo = -1;

    if ((speed != oldspeed) || (bits != oldbits) || (stereo != oldstereo))
    {
	/* Sync the dsp - otherwise strange things may happen */
#ifdef DEBUG
	fprintf(stderr, " - syncing - ");
#endif
	sync_audio();

	/* Set the sample speed, size and stereoness */
	/* We only use values of 8 and 16 for bits. This implies
	 * unsigned data for 8 bits, and little-endian signed for 16 bits.
	 */
	if (ioctl(audio, SNDCTL_DSP_SAMPLESIZE, &bits) < 0)
	    ErrDie(audev);
	if (ioctl(audio, SNDCTL_DSP_STEREO, &stereo) < 0)
	    ErrDie(audev);
	if (ioctl(audio, SNDCTL_DSP_SPEED, &speed) < 0)
	    ErrDie(audev);
    }
    oldspeed = speed; oldbits = bits; oldstereo = stereo;
}

void sync_audio(void)
{
    if (ioctl (audio, SNDCTL_DSP_SYNC, NULL) < 0)
	ErrDie(audev);
}

void cleanup_audio(void)
{
    if (ioctl (audio, SNDCTL_DSP_RESET) < 0)
	ErrDie(audev);
    close(audio);
}



