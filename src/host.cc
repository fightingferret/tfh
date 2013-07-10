#include "ctype.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <syslog.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/telnet.h>
#include <signal.h>
#include "define.h"
#include "struct.h"


pid_t      daemon_pid   = -1;
char       read_buffer  [ FOUR_LINES ];
link_data*  host_stack  = NULL;
int          read_pntr  = 0;
int              input  = -1;
int             output  = -1;


void  connect_link   ( link_data*, const char* );


void broken_pipe( )
{
  roach( "Write_Host: Pipe to host daemon is broken." );
  roach( "Write_Host: Attempting to revive daemon." );
  init_daemon( );
  return;
}


bool init_daemon( )
{
  char       tmp  [ ONE_LINE ];
  int     pipe_a  [ 2 ];
  int     pipe_b  [ 2 ];

  kill_daemon();

  printf( "Init_Daemon: Waking Daemon...\n" );

  pipe( pipe_a );
  pipe( pipe_b );  

  if( ( daemon_pid = fork( ) ) == (pid_t) 0 ) { 
    dup( pipe_a[0] );
    dup( pipe_b[1] );
    close( pipe_a[1] );
    close( pipe_b[0] );

    printf( "Init_Daemon: Pipes are %d and %d.\n",
      pipe_a[0], pipe_b[1] );

    sprintf( &tmp[0], "%d", pipe_a[0] );
    sprintf( &tmp[9], "%d", pipe_b[1] );

    if( execlp( "./daemon", "./daemon", &tmp[0], &tmp[9], (char*) 0 ) == -1 )
      printf( "Init Daemon: Error in Execlp.\n" ); 
     
    exit( 1 );
    } 
  else if( daemon_pid == -1 ) {
    return FALSE;
    }

  close( pipe_a[0] );
  close( pipe_b[1] );

  input  = pipe_b[0];
  output = pipe_a[1];

  /*
  fcntl( input,  F_SETFL, O_NDELAY );
  fcntl( output, F_SETFL, O_NDELAY );
  */

  /* CLOSE PIPE ON EXEC */

  fcntl( input,  F_SETFD, 1 );
  fcntl( output, F_SETFD, 1 );
 
  return TRUE;
}


int players_on( )
{
  int          num  = 0;
  link_data*  link;

  for( link = link_list; link != NULL; link = link->next )
    if( link->connected == CON_PLAYING )
      num++;

  return num;
}


void write_host( link_data* link, char* name )
{
  char*  tmp1  = static_string( );
  char*  tmp2  = static_string( );
  int    addr;  

  sprintf_minutes( tmp1, current_time-boot_time );
  sprintf( tmp2, "\n\r%d players on.\n\rSystem started %s ago.\n\r\
Getting site info ...\n\r", players_on( ), tmp1 );

  write( link->channel, tmp2, strlen( tmp2 ) );
 
  if( count( host_stack ) > 20 ) {
    aphid( "Write_Host: Host daemon is swamped." );
    }        
  else if( write( output, name, sizeof( struct in_addr ) ) == -1 ) {
    fprintf(stderr, "TFE: Error writing to daemon.\n");
    perror("write");
    broken_pipe( );
    }
  else {
    append( host_stack, link );
    return;
    }

  memcpy( &addr, name, sizeof( int ) );
  addr = ntohl( addr );

  sprintf( tmp1, "%d.%d.%d.%d",
    ( addr >> 24 ) & 0xFF, ( addr >> 16 ) & 0xFF,
    ( addr >>  8 ) & 0xFF, ( addr       ) & 0xFF );

  connect_link( link, tmp1 );
  return;
}
 

void read_host( )
{ 
  struct timeval  start;  
  link_data*       link;  
  int              i, j;
  int             nRead;

  gettimeofday( &start, NULL );

  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  fd_set read_fds;
  FD_ZERO(&read_fds);
  FD_SET(input, &read_fds);
  int selectStatus = select(64, &read_fds, 0, 0, &timeout);
  if ( selectStatus == -1 ) {
     fprintf(stderr, "TFE: Error reading from daemon.\n");
     perror("read");
     return;
  }
  else if ( selectStatus == 0 ) {
     return;
  }
  else { // input received
     nRead = read( input, &read_buffer[read_pntr], FOUR_LINES-read_pntr );
     if( nRead == -1 ) {
        fprintf(stderr, "TFE: Error reading from daemon.\n");
        perror("read");
        broken_pipe( );
        return;
     } 
     read_pntr += nRead;
  }

  do {
     for( i = 0; i < read_pntr; i++ )
        if( read_buffer[i] == '\0' ) // search for a null
           break;

     // If there isn't a complete hostname string in the buffer, give up.
     if( i == read_pntr )
        return;

     // i is pointing to the end of a string now

     if( strcmp( read_buffer, "Alive?" ) && host_stack != NULL ) {

        // read_buffer is a valid hostname string or IP address string

        // assumes that IP address in host_stack matches what we just read
        link       = host_stack;
        host_stack = host_stack->next;
        connect_link( link, read_buffer );
     }

     // copy remainder of the buffer i characters backwards
     for( j = i+1; j < read_pntr; j++ )
        read_buffer[j-i-1] = read_buffer[j];

     read_pntr -= i+1;
  }
  while ( read_pntr > 0 );

  pulse_time[ TIME_DAEMON ] = stop_clock( start );  

  return;
}


void connect_link( link_data* link, const char* host )
{  
  char  tmp  [ TWO_LINES ];

  link->host = alloc_string( host, MEM_LINK );
 
  link->next = link_list;
  link_list  = link;

  write_greeting( link );

  sprintf( tmp, "Connection from %s", link->host );
  info( "", LEVEL_DEMIGOD, tmp, IFLAG_LOGINS );

  return;
}


void kill_daemon()
{
   if ( daemon_pid > 0 )
   {
      printf( "Kill_Daemon: Killing hostname daemon...\n");
      if ( input != -1 ) 
      {
         close(input);
         close(output);
      }
      kill( daemon_pid, SIGTERM );
      daemon_pid = -1;

      // Purge the host_stack.
      link_data* tmp_link;
      while ( host_stack != NULL ) {
         aphid( "Kill_Daemon: Closing pending connection." );
	 if ( host_stack->channel > 0 ) {
	    close( host_stack->channel );
	 }
	 tmp_link = host_stack;
         host_stack = host_stack->next;
	 delete tmp_link;
      }
   }
   return;
}


