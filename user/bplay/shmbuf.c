/*
**
** bplay/shmbuf.c (C) David Monro 1996
**
** Copyright under the GPL - see the file COPYING in this directory
**
** 1999.02.09. huba
** Fixed a bug in semaphore handling
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#ifndef __uClinux__
#include <sys/sem.h>
#endif
#include <sys/shm.h>
#ifndef __uClinux__
#include <sys/wait.h>
#endif

#ifndef __uClinux__
#ifdef __GLIBC__
/* Nasty hack */
#ifndef SEMMSL
#define SEMMSL 32
#endif
#endif
#endif

/* The default size of the big array */
/* (currently 256K - nearly 1.5 sec at max rate) */
#define DEFAULTBUFFSIZE 0x040000

/* Types */
typedef struct blockinf_t
{
    int count;	/* How many bytes in this buffer */
    int last;	/* Should we terminate after this buffer? */
    int setit;	/* Should we re-set the audio parameters to be the ones here? */
    int speed;
    int bits;
    int stereo;
} blockinf_t;

#ifndef __uClinux__
#ifdef _SEM_SEMUN_UNDEFINED		/* is semun defined in <sys/sem.h>? */
union semun
{
	int val;			/* value for SETVAL */
	struct semid_ds *buf;		/* buffer for IPC_STAT & IPC_SET */
	unsigned short int *array;	/* array for GETALL & SETALL */
	struct seminfo *__buf;		/* buffer for IPC_INFO */
};
#endif 
#endif

/* Globals */
int bigbuffsize = DEFAULTBUFFSIZE;

/* Statics - mostly shared memory etc */
static int shmid, shmid2;
#ifndef __uClinux__
static int *disksemid = 0, *sndsemid = 0;
#endif
static char *bigbuff;
static char **buffarr;
static int numbuffs;
#ifndef __uClinux__
static int numsemblks;
#endif
static blockinf_t *buffinf;

/* prototypes */
#ifndef __uClinux__
void cleanupsems(void);
#endif
static void sighandler(int i);

/* Extern globals */
extern int abuf_size;
extern int audio;
extern char *progname;
#ifndef __uClinux__
extern pid_t pid;
#endif
extern int recorder;
extern int debug;

/* extern prototypes */
extern void ErrDie(char *err);
extern void snd_parm(int speed, int bits, int stereo);
extern void sync_audio(void);
extern void cleanup_audio(void);

void init_shm(void)
{
    int i;

    /* Round up to a multiple of abuf_size */
    if (bigbuffsize % abuf_size != 0) {
	    bigbuffsize = ((bigbuffsize / abuf_size) +1) * abuf_size;
    }

	/* Create, attach and mark for death the big buffer */
    shmid = shmget(IPC_PRIVATE, bigbuffsize,
	IPC_EXCL | IPC_CREAT | 0600);
    if (shmid == -1)
	ErrDie("shmget");
    bigbuff = shmat(shmid, IPC_RMID, SHM_RND);
    if (bigbuff == (char*)-1)
    {
	perror("shmat");
	if(shmctl(shmid, IPC_RMID, NULL))
		perror("shmctl");
	exit(-1);
    }
    memset(bigbuff, 0, bigbuffsize);
    if(shmctl(shmid, IPC_RMID, NULL))
	ErrDie("shmctl");

    /* Create an array of pointers. Point them at equally spaced
    ** chunks in the main buffer, to give lots of smaller buffers
    */
    numbuffs = bigbuffsize/abuf_size;
    buffarr = (char**)malloc(numbuffs*sizeof(char*));
    for (i=0; i<numbuffs; i++)
	buffarr[i] = bigbuff + i * abuf_size;

    /* Create a small amount of shared memory to hold the info
    ** for each buffer.
    */
    shmid2 = shmget(IPC_PRIVATE, numbuffs*sizeof(blockinf_t),
	IPC_EXCL | IPC_CREAT | 0600);
    if (shmid == -1)
	ErrDie("shmget");
    buffinf = (blockinf_t*)shmat(shmid2, IPC_RMID, SHM_RND);
    if (buffinf == (blockinf_t*)((char*)-1))
    {
	perror("shmat");
	if(shmctl(shmid2, IPC_RMID, NULL))
		perror("shmctl");
	exit(-1);
    }
    memset(buffinf, 0, numbuffs*sizeof(blockinf_t));
    if(shmctl(shmid2, IPC_RMID, NULL))
	ErrDie("shmctl");

#if USEBUFFLOCK
    if (!getuid()) {
	/* Ok, go root to lock the buffers down */
	if(setreuid(geteuid(), getuid()) == -1)
	{
	    fprintf(stderr, "%s: setreuid: %s: continuing anyway\n",
		progname, strerror(errno));
	    fprintf(stderr, "real uid = %d, effective uid = %d\n",
		getuid(), geteuid());
	}

	if(shmctl(shmid, SHM_LOCK, NULL) || shmctl(shmid2, SHM_LOCK, NULL))
	    fprintf(stderr,
		"%s: shmctl: %s: continuing with unlocked buffers\n",
		    progname, strerror(errno));

	if(setreuid(geteuid(), getuid()) == -1)
	{
	    fprintf(stderr, "%s: setreuid: %s: continuing anyway\n",
		progname, strerror(errno));
	    fprintf(stderr, "real uid = %d, effective uid = %d\n",
		getuid(), geteuid());
	}
    }

#endif
#ifndef __uClinux__
    /* Set up the appropriate number of semaphore blocks */
    numsemblks = numbuffs/SEMMSL;
    if((numsemblks * SEMMSL) < numbuffs)
	numsemblks++;
    /* Malloc arrays of semaphore ids (ints) for the semaphores */
    if ((disksemid = (int*)malloc(sizeof(int)*numsemblks)) == NULL)
	ErrDie("malloc");
    for (i=0;i<numsemblks;i++)
	disksemid[i] = -1;
    if ((sndsemid = (int*)malloc(sizeof(int)*numsemblks)) == NULL)
	ErrDie("malloc");
    for (i=0;i<numsemblks;i++)
	sndsemid[i] = -1;
    /* Create the semaphores */
    for (i=0;i<numsemblks;i++)
    {
	if ((disksemid[i] = semget(IPC_PRIVATE, SEMMSL,
	    IPC_EXCL | IPC_CREAT | 0600)) == -1)
	    ErrDie("semget");
	if ((sndsemid[i] = semget(IPC_PRIVATE, SEMMSL,
	    IPC_EXCL | IPC_CREAT | 0600)) == -1)
	    ErrDie("semget");
    }
#endif
    /* Catch some signals, so we clean up semaphores */
    signal(SIGINT, sighandler);
}


#ifndef __uClinux__
/* Does an up on the appropriate semaphore */
void up(int *semblk, int xsemnum)
{
    struct sembuf sbuf;

    sbuf.sem_num = xsemnum%SEMMSL;
    sbuf.sem_op = 1;
    sbuf.sem_flg = 0;

    while (semop(semblk[xsemnum/SEMMSL], &sbuf, 1) == -1)
      if (errno != EINTR) {
	perror("semop");
	break;
      }
}

/* Does a down on the appropriate semaphore */
void down(int *semblk, int xsemnum)
{
    struct sembuf sbuf;

    sbuf.sem_num = xsemnum%SEMMSL;
    sbuf.sem_op = -1;
    sbuf.sem_flg = 0;

    while (semop(semblk[xsemnum/SEMMSL], &sbuf, 1) == -1)
      if (errno != EINTR) {
	perror("semop");
	break;
      }
}
#endif

#ifdef __uClinux__
void diskwrite(int outfd, int cbuff)
#else
/* The recording function */
void shmrec(int outfd, long long int totalcount, int terminate)
{

    sync();

    pid = fork();
    if (pid == 0)
#endif
    {
#ifndef __uClinux__
	long int cbuff = 0;

	/* Uncatch the signals */
	signal(SIGINT, SIG_DFL);

	/* Child process writes the disk */
	while(1)
	{
#endif
	    long int count, numwr, trgt;
	    char *tmpptr;

#ifndef __uClinux__
	    /* Grab the buffer. Blocks till it is OK to do so. */
	    down(disksemid, cbuff);
#endif
	    /* Spit it out */
	    tmpptr = buffarr[cbuff];
	    numwr = 0;
	    trgt = buffinf[cbuff].count;
	    while ( (numwr < trgt) &&
	    ((count = write(outfd, tmpptr, trgt - numwr)) > 0) )
	    {
		numwr += count;
		tmpptr += count;
	    }
#ifndef __uClinux__
	    /* Mark the buffer as clean */
	    up(sndsemid, cbuff);
#endif
	    /* If the block was marked as the last one, stop */
	    if (buffinf[cbuff].last)
#ifdef __uClinux__
	    	close(outfd);
#else
		break;
#endif
#ifndef __uClinux__
	    /* Advance the pointer */
	    cbuff++;
	    cbuff%=numbuffs;
	}
	/* Tidy up and exit, we are being waited for */
	close(outfd);
	exit(0);
#endif
    }
#ifdef __uClinux__
void shmrec(int outfd, long long int totalcount, int terminate)
#else
    else
#endif
    {
	/* Parent reads audio */
	long long int cbuff = 0, totalrd = 0;
#ifdef __uClinux__
	sync();
#endif
	while (totalrd < totalcount)
	{
	    long long int trgt, count, numrd;
	    char *tmpptr;
	    trgt = totalcount - totalrd;
	    if (trgt > abuf_size)
		trgt = abuf_size;
#ifndef __uClinux__
	    /* Get the buffer. Blocks until OK to do so */
	    down(sndsemid, cbuff);
#endif
	    /* Read a block of data */
	    numrd = 0;
	    tmpptr = buffarr[cbuff];
	    while( (numrd < trgt) &&
		((count = read(audio, tmpptr, trgt - numrd)) > 0) )
	    {
		numrd += count;
		tmpptr += count;
	    }
	    /* Update the count for this block */
	    buffinf[cbuff].count = numrd;
	    /* Update the amount done */
	    totalrd += numrd;
	    /* Tell the reader to stop if needed */
	    if ((totalrd >= totalcount) && terminate)
		buffinf[cbuff].last = 1;
#ifdef __uClinux__
	    diskwrite(outfd, cbuff);
#else
	    /* Mark the buffer dirty */
	    up(disksemid, cbuff);
#endif
	    /* Update the counter */
	    cbuff++;
	    cbuff%=numbuffs;
	}
	/* Tidy up and wait for the child */
	close(audio);
#ifndef __uClinux__
	wait(NULL);
	
	/* Free all the semaphores */
	cleanupsems();
#endif
    }
#ifndef __uClinux__
}
#endif

#ifdef __uClinux__
void audiowrite(int cbuff);
#endif
void diskread(int infd, long long int totalplay, char hd_buf[20],
    int terminate, int speed, int bits, int stereo, int jump)
{

    int count, i, limited = 0;
    char *tmppt;
    int numread;
    long long int totalread = 0;
    int first = 1;

    static int triggered = 0;	/* Have we let the writer go? */
    static int cbuff = 0;	/* Which buffer */

    if (jump)
    {
        jump = jump * speed * (bits/8) * (stereo * 2);

        if (lseek (infd, jump, SEEK_CUR) < jump)
            fprintf(stderr, "couldn't jump %d bytes\n", jump);
    }

    if (totalplay) limited = 1;
    if (totalplay == -1)
    {
	totalplay = 0;
	limited = 1;
    }

    while (1)
    {
	int trgt;

#ifndef __uClinux__
	/* Wait for a clean buffer */
	down(disksemid, cbuff);
#endif
	/* Read from the input */
	numread = 0;
	trgt = abuf_size;
	if (limited && (totalread + trgt > totalplay))
	    trgt = totalplay - totalread;
	tmppt = buffarr[cbuff];
	if(first && trgt)
	{
	    buffinf[cbuff].setit = 1;
	    buffinf[cbuff].speed = speed;
	    buffinf[cbuff].bits = bits;
	    buffinf[cbuff].stereo = stereo;
	    if(hd_buf)
	    {
		memcpy(tmppt, hd_buf, 20);
		tmppt += 20; numread = 20;
	    }
	    first = 0; 
	}
	while ( (numread < trgt) &&
	    ((count = read(infd, tmppt, trgt - numread)) != 0) )
	{
	    tmppt += count; numread += count;
	}
	if (debug >= 2)
	    fprintf(stderr, "in:%d, %d\n", cbuff, numread);

	/* Update the count for this block */
	buffinf[cbuff].count = numread;
	totalread += numread;
	/* Was it our last block? */
	if (numread < abuf_size)
	    break;
	if(triggered)
#ifdef __uClinux__
	    audiowrite(cbuff);
#else
	    up(sndsemid, cbuff);
#endif
	else
	    if(cbuff == numbuffs-1)
	    {
	      if (debug)
	            fprintf(stderr, "Triggering (in loop)\n");

		for(i = 0; i < numbuffs; i++)
#ifdef __uClinux__
		    audiowrite(i);
#else
		    up(sndsemid,i);
#endif
		    triggered = 1;
	    }
	/* Update counter */
	cbuff++;
	cbuff %= numbuffs;
    }
    /* Finish off this set of buffers */
    if(terminate)
    {
	buffinf[cbuff].last = 1;
	if(!triggered) {
	    if (debug)
	        fprintf(stderr, "Triggering (after loop, partial)\n");

	    /* If it wasn't triggered, we haven't filled past cbuff */
	    for(i = 0; i < cbuff; i++)
#ifdef __uClinux__
	    	audiowrite(i);
#else
		up(sndsemid, i);
#endif
	}
#ifdef __uClinux__
	audiowrite(cbuff);
#else
	up(sndsemid, cbuff);
#endif
    }
    else if((!triggered) && (cbuff == numbuffs-1))
    {
        if (debug)
            fprintf(stderr, "Triggering (after loop, full)\n");

	for(i = 0; i < numbuffs; i++)
#ifdef __uClinux__
	    audiowrite(i);
#else
	    up(sndsemid,i);
#endif
	    triggered = 1;
    }
    else if(triggered)
#ifdef __uClinux__
	audiowrite(cbuff);
#else
	up(sndsemid,cbuff);
#endif
    cbuff++;
    cbuff %= numbuffs;
}

#ifdef __uClinux__
void audiowrite(int cbuff)
#else
volatile void audiowrite(void)
#endif
{
#ifndef __uClinux__
    int cbuff = 0;
#endif
    int count, numwr, trgt;
    char *tmpptr;

#ifndef __uClinux__
    /* Uncatch the signals, so we don't clean up twice */
    signal(SIGINT, SIG_DFL);

    /* Child process writes the audio */
    while(1)
    {
	/* Wait for dirty buffer */
	down(sndsemid, cbuff);
#endif
	/* Spit to the audio device */
	if(buffinf[cbuff].setit)
	{
	    snd_parm(buffinf[cbuff].speed, buffinf[cbuff].bits,
		buffinf[cbuff].stereo);
	    buffinf[cbuff].setit = 0;
	}
	trgt = buffinf[cbuff].count;
	numwr = 0;
	tmpptr = buffarr[cbuff];
	while ( (numwr < trgt) &&
	    ((count = write(audio, tmpptr, trgt - numwr)) > 0) )
	{
	    if (count == -1)
		ErrDie("write");
	    numwr += count;
	    tmpptr += count;
	}
	if (debug >= 2)
            fprintf(stderr, "out:%d, %d\n", cbuff, numwr);

	/* Was it the last buffer? */
	if (buffinf[cbuff].last)
	{
#ifdef __uClinux__
	    sync_audio();
	    close(audio);
#else
	    up(disksemid, cbuff);	/* Not really needed */
	    break;
#endif
	}
#ifndef __uClinux__
	/* Mark as clean */
	up(disksemid, cbuff);
	/* Update counter */
	cbuff++;
	cbuff %= numbuffs;
    }
    /* Tidy up and be reaped */
    sync_audio();
    close(audio);
    exit(0);
#endif
}

#ifndef __uClinux__
void initsems(int disks, int snds)
{
    int i,j;
    union semun dsu, ssu;

    dsu.val = disks;
    ssu.val = snds;
    for (i=0;i<numsemblks;i++)
	for (j=0; j<SEMMSL;j++)
	{
	    if(semctl(disksemid[i], j, SETVAL, dsu) == -1)
		ErrDie("semctl");
	    if(semctl(sndsemid[i], j, SETVAL, ssu) == -1)
		ErrDie("semctl");
	}
}
	
void cleanupsems(void)
{
    int i;
    union semun s;

    s.val = 0;

    if (disksemid)
	for (i = 0; i < numsemblks; i++)
	    if (disksemid[i] != -1)
		semctl(disksemid[i], 0, IPC_RMID, s);
    if (sndsemid)
	for (i = 0; i < numsemblks; i++)
	    if (sndsemid[i] != -1)
		semctl(sndsemid[i], 0, IPC_RMID, s);
}
#endif

static void sighandler(int i)
{
    if (debug)
        fprintf(stderr, "signal %d received, cleaning up.\n", i);

#ifndef __uClinux__
    if (!recorder && pid)
        kill(pid,9);
#endif
    cleanup_audio();
#ifndef __uClinux__
    cleanupsems();
#endif
    exit(1);
}
