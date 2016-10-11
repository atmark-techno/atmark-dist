
#include	<stdio.h> 
#include 	<stdlib.h> 
#include 	<string.h> 
#include 	<unistd.h> 
#include 	<fcntl.h> 
#include 	<sys/ioctl.h> 

#ifdef WDT_DEBUG
#define		DBG_L( )	printf( "[%d]:\n", __LINE__ ) 
#define		DBG( args... )	printf( args ) ; printf( "\n" ) 
#else 
#define		DBG_L( )    
#define		DBG( args... )
#endif

#define		T_INTERVAL	(30) 

int main( int argc, const char *argv[] )
{ 
	int t_sleep = T_INTERVAL ;
	int fd = open( "/dev/watchdog", O_WRONLY ) ; 
	DBG_L( ) ; 
	
	if( fd == -1 ) { 
		perror( "watchdog" ) ; 
		DBG_L( ) ; 
		exit( 1 ) ; 
	} 

	while( 1 ) { 
        	write( fd, "\0", 1 ) ; 
		fsync( fd ) ; 
		DBG_L( ) ; 
		sleep( t_sleep ) ; 
		DBG( "t_sleep:%d\n", t_sleep ) ; 
	} 

 exit_proc:
	return 0 ; 
	
} /* main */ 
