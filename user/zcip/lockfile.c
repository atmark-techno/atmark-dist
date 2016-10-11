
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include "lockfile.h"

#define LOCKDIR   	"/var/run"
#define LOCKPREFIX	"/zcip."

static void
gen_lockfile_name(char * nbuf, char * device)
{
   char * p;
   
   p = strrchr(device, '/');
   strcpy(nbuf, LOCKDIR LOCKPREFIX);
   if( p ) strcat(nbuf, p+1);
   else    strcat(nbuf, device);
}

int
lock_check(char * device)
{
   FILE * fd;
   char nbuf[128];
   int i;

   gen_lockfile_name(nbuf, device);
   fd = fopen(nbuf, "r");
   if( fd )
   {
      fscanf(fd, "%d", &i);
      fclose(fd);
      if( kill(i, 0) == 0 ) return 0;   /* Sorry */
      if( errno == EPERM ) return 0;
      if( unlink(nbuf) == -1 ) return 0;
   }
   fd = fopen(nbuf, "w"); /* Got it! */
   i = getpid();
   fprintf(fd, "%d\n", i);
   fclose(fd);
   return 1;
}

void
lock_unlock(char * device)
{
   FILE * fd;
   char nbuf[128];
   int i;

   gen_lockfile_name(nbuf, device);
   fd = fopen(nbuf, "r");
   if( fd )
   {
      fscanf(fd, "%d", &i);
      fclose(fd);
      if( i == getpid() || kill(i, 0) != 0 )
         unlink(nbuf);
   }
}
