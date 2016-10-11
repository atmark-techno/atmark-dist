/*
** brec/bplay.c (C) David Monro 1996
**
** Copyright under the GPL - see the file COPYING in this directory
**
**
*/
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#ifndef __uClinux__
#include <sys/wait.h>
#endif

#include <sys/time.h>
#include <sys/resource.h>

#include <sys/soundcard.h>

/* Needed for BYTE_ORDER and BIG/LITTLE_ENDIAN macros. */
#ifndef _BSD_SOURCE
# define _BSD_SOURCE
# include <endian.h>
# undef  _BSD_SOURCE
#else
# include <endian.h>
#endif

#include <sys/types.h>
#include <byteswap.h>

/* Adapted from the byteorder macros in the Linux kernel. */
#if BYTE_ORDER == LITTLE_ENDIAN
#define cpu_to_le32(x) (x)
#define cpu_to_le16(x) (x)
#else
#define cpu_to_le32(x) bswap_32((x))
#define cpu_to_le16(x) bswap_16((x))
#endif

#define le32_to_cpu(x) cpu_to_le32((x))
#define le16_to_cpu(x) cpu_to_le16((x))

#include "fmtheaders.h"

/* Types and constants */
typedef enum sndf_t {F_UNKNOWN, F_WAV, F_VOC, F_RAW} sndf_t;

#define MSPEED	1
#define MBITS	2
#define MSTEREO	4

/* Globals */
char *progname;
#ifndef __uClinux__
int forked;
pid_t pid;
#endif
int recorder = 0;
int debug = 0;
int verbose = 1;

/* Prototypes */


/* From my <string.h>. I wonder why it doesn't work there?!? */
extern char *basename __P ((__const char *__filename));



void Usage(void);
void ErrDie(char *err);
void Die(char *err);

void cleanup(int val, void *arg);

void getbcount(int speed, int bits, int stereo, long long int *bcount,
	int timelim, int samplim, int timejmp, int sampjmp, int *bjump);
void playraw(int thefd, char hd_buf[20], int speed, int bits, int stereo,
	int jump, int secs);
void playwav(int thefd, char hd_buf[20], int mods, int speed, int bits,
	int stereo, int jump, int secs);
void playvoc(int thefd, char hd_buf[20]);

/* extern globals */
extern int audio, abuf_size, fmt_mask;
extern char *audev;
extern int bigbuffsize;

/* extern prototypes */
extern void init_sound(int recorder);
extern void snd_parm(int speed, int bits, int stereo);
extern void init_shm(void);
extern void shmrec(int outfd, long long int bcount, int terminate);
extern void diskread(int outfd, long long int bcount, char hd_buf[20], int terminate,
    int speed, int bits, int stereo, int jump);
#ifndef __uClinux__
extern volatile void audiowrite(void);
extern void initsems(int disks, int snds);
extern void cleanupsems(void);
#endif


int main(int argc, char *argv[])
{

	int thefd;			/* The file descriptor */
	int speed, bits, stereo;	/* Audio parameters */
	int timelim;			/* Recording time in secs */
	int samplim;			/* Recording time in samples */
	int timejmp;			/* Skip time in secs */
	int sampjmp;			/* Skip time in samples */
	long long int bcount;			/* Number of bytes to record */
	int bjump;			/* Number of bytes to skip */
	int themask;			/* Permission mask for file */
	sndf_t filetype;		/* The file type */
	int mods;			/* So user can override */
	int optc;			/* For getopt */

	progname = basename(argv[0]);	/* For errors */

	/* Ok, find out if we record or play */
	if (strcmp(progname, "brec") == 0)
	    recorder = 1;
	else
	{
	    if (!(strcmp(progname, "bplay") == 0))
		Die("must be called as bplay or brec");
	    else
		recorder = 0;
	}
	/* Default values */
	speed = 8000; bits = 8; stereo = 0;
	timelim = 0; samplim = 0; bcount = 0;
	timejmp = 0; sampjmp = 0; bjump = 0;
	filetype = F_UNKNOWN;
	mods = 0;
	/* Parse the options */
	while ((optc = getopt(argc, argv, "Ss:b:t:T:j:J:rvwd:B:D:q")) != -1)
	{
		switch(optc)
		{
		case 's':
			speed = atoi(optarg);
			if (speed < 300)
				speed *= 1000;
			mods |= MSPEED;
			break;
		case 'b':
			bits = atoi(optarg);
			if ((bits != 8) && (bits != 16))
				Usage();
			mods |= MBITS;
			break;
		case  'S':
			stereo = 1;
			mods |= MSTEREO;
			break;
		case 't':
			timelim = atoi(optarg);
			break;
		case 'T':
			samplim = atoi(optarg);
			break;
		case 'j':
			timejmp = atoi(optarg);
			break;
		case 'J':
			sampjmp = atoi(optarg);
			break;
		case 'q':
		        verbose = 0;
			break;
		case 'r':
			filetype = F_RAW;
			break;
		case 'v':
			filetype = F_VOC;
			break;
		case 'w':
			filetype = F_WAV;
			break;
		case 'd':
			audev = optarg;
			break;
		case 'B':
			bigbuffsize = atoi(optarg);
			break;
		case 'D':
		        debug = atoi(optarg);
			break;
		default:
			Usage();
		}
	}

#if 1
	/* This program is either set-uid or run by root (I hope...) */
	if (!getuid()) {
	    if (setpriority(PRIO_PROCESS, 0, -20) == -1) fprintf(stderr,
		    "%s: setpriority: %s: continuing anyway\n",
		    progname, strerror(errno));
	}

#endif
	/* Drop out of suid mode before we open any files */
	if(setreuid(geteuid(), getuid()) == -1)
	{
		fprintf(stderr, "%s: setreuid: %s: continuing anyway\n",
			progname, strerror(errno));
		fprintf(stderr, "real uid = %d, effective uid = %d\n",
			getuid(), geteuid());
	}

	/* Calculate the time limit in samples if need be */
	if (optind > argc - 1)
	{
	    if(recorder)	/* No file, so stdin or stdout */
		thefd = 1;
	    else
		thefd = 0;
	}
	else
	{
	    /* Open a file */
	    if(recorder)
	    {
		/* Ok, get the mask for the opening the file */
		themask = umask(0);
		umask(themask);
		if ((thefd = open(argv[optind], O_CREAT | O_TRUNC | O_WRONLY | O_LARGEFILE,
		    (~themask) & 0666)) == -1)
		    ErrDie(argv[optind]);
	    }
	    else
		if ((thefd = open(argv[optind], O_RDONLY | O_LARGEFILE)) == -1)
		    ErrDie(argv[optind]);
	}

	/* Open and set up the audio device */
	init_sound(recorder);

	/* Check if the card is capable of the requested operation */
	/*
	** Can't check for stereo yet, just number of bits. Also,
	** can't check for 12 bit operations yet.
	*/

#if 0
	if ((bits == 8) & !(fmt_mask & AFMT_U8)) Die("Format not supported by audio device");
	if ((bits == 16) & !(fmt_mask & AFMT_S16_BE)) Die("Format not supported by audio device");
#endif

	/* Set up the shared buffers and semaphore blocks */
	on_exit(cleanup, 0);
	init_shm(); /* MUST be called after init_sound() */

	/* Call the appropriate routine */
	if (recorder)
	{
		if ((timelim == 0)  && (samplim == 0)) {
			bcount = INT64_MAX - 1;
		} else {
			getbcount(speed, bits, stereo, &bcount, timelim,
					samplim, timejmp, sampjmp,
					&bjump);
		}
		if (debug)
		        fprintf(stderr, "bcount: %lld\n", bcount);

		if (filetype == F_UNKNOWN)
			filetype = F_RAW;	/* Change to change default */
		switch(filetype)
		{
		case F_WAV:
			/* Spit out header here... */
		  if (verbose) {
		    fprintf(stdout, "Writing MS WAV sound file");
		    fprintf(stdout, ", %dHz, %dbit, %s\n", speed, bits, (stereo)? "stereo":"");
		  }
			{
				wavhead header;
				unsigned long long int tmp;

				char *riff = "RIFF";
				char *wave = "WAVE";
				char *fmt = "fmt ";
				char *data = "data";

				memcpy(&(header.main_chunk), riff, 4);
				if ( (tmp = sizeof(wavhead) - 8 + bcount) >> 32 ) {
				   header.length =  0xFFFFFFFF; // do not overload the header
				   fprintf(stderr, " (WARNING: Resulting file size is larger than 4GiB, violating the WAVE format specification!)\n");
				}
				else
				   header.length = cpu_to_le32(tmp);

				memcpy(&(header.chunk_type), wave, 4);

				memcpy(&(header.sub_chunk), fmt, 4);
				header.sc_len = cpu_to_le32(16);
				header.format = cpu_to_le16(1);
				header.modus = cpu_to_le16(stereo + 1);
				header.sample_fq = cpu_to_le32(speed);
				header.byte_p_sec = cpu_to_le32(((bits > 8)?
							2:1)*(stereo+1)*speed);
				header.byte_p_spl = cpu_to_le16(((bits > 8)?
							2:1)*(stereo+1));
				header.bit_p_spl = cpu_to_le16(bits);

				memcpy(&(header.data_chunk), data, 4);
				header.data_length = ( bcount >> 32 ) ? 0xFFFFFFFF : cpu_to_le32(bcount) ; //FIXME see above, make sure that it works cleanly
				write(thefd, &header, sizeof(header));
			}
		case F_RAW:
			if (filetype == F_RAW)
			  if (verbose) {
			    fprintf(stdout, "Writing raw sound file");
			    fprintf(stdout, ", %dHz, %dbit, %s\n", speed, bits, (stereo)? "stereo":"");
			  }
			snd_parm(speed, bits, stereo);
#ifndef __uClinux__
			initsems(0, 1);
#endif
			shmrec(thefd, bcount, 1);
			break;
		case F_VOC:
			/* Spit out header here... */
		  if (verbose) {
		    fprintf(stdout, "Writing CL VOC sound file");
		    fprintf(stdout, ", %dHz, %dbit, %s\n", speed, bits, (stereo)? "stereo":"");}
			{
				vochead header;
				blockTC ablk;
				blockT9 bblk;
				int i;
				char fill = 0;

				for (i=0;i<20;i++)
					header.Magic[i] = VOC_MAGIC[i];
				header.BlockOffset = cpu_to_le16(0x1a);
				header.Version = cpu_to_le16(0x0114);
				header.IDCode = cpu_to_le16(0x111F);
				write(thefd, &header, sizeof(vochead));

				snd_parm(speed, bits, stereo);
#ifndef __uClinux__
				initsems(0, 1);
#endif

				i = bcount;
				if (bcount >= 0xFFFFF2)
				{
				   i = 0xFFFFF2 + 12;
				   fprintf(stderr, "Warning: length is out of allowed range, consider using another sound format!\n");
				}

				ablk.BlockID = 9;
				ablk.BlockLen[0] = (i + 12) & 0xFF;
				ablk.BlockLen[1] = ((i + 12) >> 8) & 0xFF;
				ablk.BlockLen[2] = ((i + 12) >> 16) & 0xFF;
				bblk.SamplesPerSec = cpu_to_le32(speed);
				bblk.BitsPerSample = bits;
				bblk.Channels = stereo + 1;
				bblk.Format = cpu_to_le16((bits == 8)? 0 : 4);
				write(thefd, &ablk, sizeof(ablk));
				write(thefd, &bblk, sizeof(bblk));
				shmrec(thefd, i, 1);
				write(thefd, &fill, 1);
			}
			break;
		default:
			Die("internal error - fell out of switch");
		}
	}
	else
	{
		int count;
		char hd_buf[20];	/* Holds first 20 bytes */

		count = read(thefd, hd_buf, 20);
		if (count < 0) ErrDie("read");
		if (count < 20) Die("input file less than 20 bytes long.");

#ifndef __uClinux__
		initsems(1, 0);

		pid = fork();
		if(!pid)
			audiowrite();	/* Doesn't return */
		forked = 1;
#endif

		/* Pick the write output routine */
		if(strstr(hd_buf, VOC_MAGIC) != NULL)
			playvoc(thefd, hd_buf);
		else if(strstr(hd_buf, "RIFF") != NULL)
      {
          if (sampjmp)
              playwav(thefd, hd_buf, mods, speed, bits, stereo, sampjmp, 0);
          else
              playwav(thefd, hd_buf, mods, speed, bits, stereo, timejmp, 1);
      }
		else /* Assume raw data */
          if (sampjmp)
              playraw(thefd, hd_buf, speed, bits, stereo, sampjmp, 0);
          else
              playraw(thefd, hd_buf, speed, bits, stereo, timejmp, 1);


#ifndef __uClinux__
		wait(NULL);
		cleanupsems();
#endif
	}

	return -1;  /* We should never reach this, but lets keep gcc happy */
}

void Usage(void)
{
	fprintf(stderr,
		"Usage: %s [-d device] [-B buffersize] [-S] [-s Hz] [-b 8|16] [-t secs] [-q] [-D level] [-r|-v|-w] [filename]\n",
		progname);
	exit(1);
}

void ErrDie(char * err)
{
	fprintf(stderr, "%s: %s: %s\n", progname, err, strerror(errno));
	exit(-1);
}

void Die(char * err)
{
	fprintf(stderr, "%s: %s\n", progname, err);
#ifndef __uClinux__
	if (forked) { kill(pid,9); }
#endif
	exit(-1);
}

void cleanup(int val, void *arg)
{
#ifndef __uClinux__
	cleanupsems();
#endif
}

void getbcount(int speed, int bits, int stereo, long long int *bcount,
	int timelim, int samplim, int timejmp, int sampjmp, int *bjump)
{
	if(timelim)
	{
		*bcount = (long long) speed * (long long) timelim * (bits/8);
		if (stereo) *bcount <<= 1;
	}
	if(samplim)
	{
		*bcount = samplim*(bits/8);
		if (stereo) *bcount <<= 1;
	}
	if(timejmp)
	{
		*bjump = speed*timejmp*(bits/8);
		if (stereo) *bjump <<= 1;
	}
	if(sampjmp)
	{
		*bjump = sampjmp*(bits/8);
		if (stereo) *bjump <<= 1;
	}
}

void playraw(int thefd, char hd_buf[20], int speed, int bits, int stereo, int jump, int secs)
{
  if (verbose) {
    fprintf(stdout, "Playing raw data : %d bit, Speed %d %s ...\n",
	    bits, speed, (stereo)? "Stereo" : "Mono");
  }

    if (secs == 0)
        jump = jump / speed;

    diskread(thefd, 0, hd_buf, 1, speed, bits, stereo, jump);
}

void playwav(int thefd, char hd_buf[20], int mods, int speed, int bits, int stereo, int jump, int secs)
{
    wavhead wavhd;
    int count;

    memcpy((void*)&wavhd, (void*)hd_buf, 20);
    count = read(thefd, ((char*)&wavhd)+20, sizeof(wavhd) - 20);

wavhd.length =  le32_to_cpu (wavhd.length);
wavhd.sc_len =  le32_to_cpu (wavhd.sc_len);
wavhd.format =  le16_to_cpu (wavhd.format);
wavhd.modus  =  le16_to_cpu (wavhd.modus);

wavhd.sample_fq  =  le32_to_cpu (wavhd.sample_fq);
wavhd.byte_p_sec =  le32_to_cpu (wavhd.byte_p_sec);

wavhd.byte_p_spl =  le16_to_cpu (wavhd.byte_p_spl);
wavhd.bit_p_spl  =  le16_to_cpu (wavhd.bit_p_spl);

wavhd.data_chunk =  le32_to_cpu (wavhd.data_chunk);
wavhd.data_length =  le32_to_cpu (wavhd.data_length);

    if(wavhd.format != 1) Die("input is not a PCM WAV file");
    if (! (mods&MSPEED))
      speed = wavhd.sample_fq;
    if (! (mods&MBITS))
      bits = wavhd.bit_p_spl;
    if (! (mods&MSTEREO))
      stereo = wavhd.modus - 1;
    if (verbose) {
      fprintf(stdout, "Playing WAVE : %d bit, Speed %d %s ...\n",
	    bits, speed, (stereo)? "Stereo" : "Mono");
    }

    if (secs == 0)
        jump = jump / speed;

    diskread(thefd, 0, NULL, 1, speed, bits, stereo, jump);
}

void playvoc(int thefd, char hd_buf[20])
{
    int count;
    int speed=0, bits=0, stereo=0;
    int inloop=0, loop_times;
    long long bytecount, loop_pos=0;
    vochead vochd;
    blockTC ccblock;
    int lastblocktype = -1;
    int quit = 0;

    if (verbose) fprintf(stdout, "Playing Creative Labs Voice file ...\n");
    memcpy((void*)&vochd, (void*)hd_buf, 20);
    count = read(thefd, ((char*)&vochd)+20, sizeof(vochd) - 20);

    vochd.BlockOffset = le16_to_cpu(vochd.BlockOffset);
    vochd.Version = le16_to_cpu(vochd.Version);
    vochd.IDCode = le16_to_cpu(vochd.IDCode);

    if (verbose) {
      fprintf(stdout, "Format version %d.%d\n", vochd.Version>>8,
	vochd.Version&0xFF);
    }
    if (vochd.IDCode != (~vochd.Version+0x1234))
	fprintf(stderr, "Odd - version mismatch - %d != %d\n",
	    vochd.IDCode, ~vochd.Version+0x1234);
    if(sizeof(vochd) < vochd.BlockOffset)
    {
	int off = vochd.BlockOffset - sizeof(vochd);
	char *junk;
	junk = (char*) malloc(off);
	read(thefd, junk, off);
    }
    while(!quit)
    {
    if ((read(thefd, (char*)&ccblock, sizeof(ccblock))) == -1)
    {
        if (debug)
	    fprintf(stderr, "Terminating\n");

	diskread(thefd, -1, NULL, 1, speed, bits, stereo, 0);
	quit = 1;
	continue;
    }
    if (debug)
        fprintf(stderr, "Block of type %d found\n", ccblock.BlockID);

    switch(ccblock.BlockID)
    {
    case 1:
	{
	blockT1 tblock;
	read(thefd, (char*)&tblock, sizeof(tblock));
	if(tblock.PackMethod != 0) Die("Non PCM VOC block");
	if (lastblocktype != 8)
	{
	    speed = 256000000/(65536 - (tblock.TimeConstant << 8));
	    bits = 8;
	    stereo = 0;
	}
	bytecount = DATALEN(ccblock) -2;
	diskread(thefd, bytecount, NULL, 0, speed, bits, stereo, 0);
	lastblocktype = 1;
	}
	break;
    case 8:
	{
	blockT8 tblock;
	read(thefd, (char*)&tblock, sizeof(tblock));

	tblock.TimeConstant = le16_to_cpu(tblock.TimeConstant);

	if(tblock.PackMethod != 0) Die("Non PCM VOC block");
	speed = 256000000/(65536 - tblock.TimeConstant);
	bits = 8;
	stereo = tblock.VoiceMode;
	if (stereo) speed >>=1;
	lastblocktype = 8;
	}
	break;
    case 9:
	{
	blockT9 tblock;
	read(thefd, (char*)&tblock, sizeof(tblock));

	tblock.SamplesPerSec = le32_to_cpu(tblock.SamplesPerSec);
	tblock.Format = le16_to_cpu(tblock.Format);

	if(tblock.Format != 0 && tblock.Format != 4)
	    Die("Non PCM VOC block");
	speed = tblock.SamplesPerSec;
	bits = tblock.BitsPerSample;
	stereo = tblock.Channels - 1;
	bytecount = DATALEN(ccblock) - 12;
	diskread(thefd, bytecount, NULL, 0, speed, bits, stereo, 0);
	lastblocktype = 9;
	}
	break;
    case 0:
        if (debug)
	    fprintf(stderr, "Terminating\n");

	diskread(thefd, -1, NULL, 1, speed, bits, stereo, 0);
	quit = 1;
	break;
    case 6:
	inloop = 1;
	read(thefd, (char*)&loop_times, 2);
	loop_times++;
	if (debug)
	    fprintf(stderr, "Beginning loop %d\n", loop_times);

	loop_pos = lseek(thefd, 0, SEEK_CUR);
	if(loop_pos == -1)
	{
	    fprintf(stderr, "Input not seekable - loop will only be played once\n");
	    loop_times = 1;
	}
	lastblocktype = ccblock.BlockID;
	break;
    case 7:
	if(!inloop)
	{
	    fprintf(stderr, "Loop end with no loop start - ignoring\n");
	    break;
	}
	if(loop_times != 0xFFFF) --loop_times;
	if(loop_times)
	{
	    if (debug)
	        fprintf(stderr, "Looping...\n");

	    lseek(thefd, loop_pos, SEEK_SET);
	}
	else
	    inloop = 0;
	lastblocktype = ccblock.BlockID;
	break;
    default:
	{
	int rd = 0, trgt = BUFSIZ;
	char junkbuf[BUFSIZ];

	fprintf(stderr, "Ignored\n");
	bytecount = DATALEN(ccblock);
	while(rd < bytecount)
	{//FIXME rd, trgt, not sure what this has to do with bytecount
	    if (rd + trgt > bytecount)
		trgt = bytecount - rd;
	    count = read(thefd, junkbuf, trgt);
	    if (count < 0) ErrDie("read");
	    if (count == 0) Die("premature eof in input");
	    rd += count;
	}
	lastblocktype = ccblock.BlockID;
	}
	break;
    }
    }
}
