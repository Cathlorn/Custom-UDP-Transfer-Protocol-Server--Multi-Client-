#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/wait.h>

#include <pthread.h>

#define BUFFSIZE 320

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>

static struct sockaddr_in echoserver;
static int sock;
static int runThreads = 1;

void *read_thread ( void *ptr );
void *write_thread ( void *ptr );

void ErrorHandler ( char *mess )
{
    perror ( mess );
    exit ( 1 );
}

void termination_handler ( int signum )
{
    if ( signum == SIGPIPE )
    {
        runThreads = 0;
    }
}

static struct sigaction new_action; //, old_action;
void SetupSignalHandlers()
{
    //Setup signal handlers
    /* Set up the structure to specify the new action. */
    new_action.sa_handler = termination_handler;

    sigemptyset ( &new_action.sa_mask );

    new_action.sa_flags = 0;

    //if (old_action.sa_handler != SIG_IGN)
    sigaction ( SIGPIPE, &new_action, NULL );

    //sigaction (SIGKILL, &new_action, NULL);
    //sigaction (SIGSTOP, &new_action, NULL);
    //sigaction (SIGTERM, &new_action, NULL);
}

void socket_init ( int port )
{
    /* Create the TCP socket */
    if ( ( sock = socket ( PF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) < 0 )
    {
        ErrorHandler ( "Failed to create socket" );
    }

    /* Construct the server sockaddr_in structure */
    memset ( &echoserver, 0, sizeof ( echoserver ) ); /* Clear struct */

    echoserver.sin_family = AF_INET;                  /* Internet/IP */

    echoserver.sin_port = htons ( port ); /* server port */

    memset ( &echoserver.sin_addr, 0, sizeof ( echoserver.sin_addr ) );

    /*Bind connection */
    if ( bind ( sock, ( struct sockaddr * ) &echoserver, sizeof ( echoserver ) ) )
        ErrorHandler ( "Failed to bind to port with server" );
}

void server_init ( int port )
{
    SetupSignalHandlers();
    socket_init ( port );
}

struct UdpCommHeader
{
    unsigned char status;
    unsigned char mgmt;
    unsigned short seqNum;
    unsigned char reserved[8];
};

void server_listen ()
{
//    socklen_t echoServerAddrLength = sizeof ( struct sockaddr_in );

    pthread_t thread1, thread2;
    char *message1 = "Thread 1";
    int  iret1;
    int  sock_flags;

    // put client socket into nonblocking mode
    sock_flags = fcntl ( sock, F_GETFL, 0 );

    fcntl ( sock, F_SETFL, sock_flags | O_NONBLOCK );

    /* Create independent threads each of which will execute function */
    //iret1 = pthread_create ( &thread1, NULL, read_thread, ( void* ) message1 );

    //iret1 = pthread_create ( &thread2, NULL, write_thread, ( void* ) message1 );

    //pthread_join ( thread1, NULL );

    //pthread_join ( thread2, NULL );

    int readData = 0;
    int bytes = 0;

    //while ( runThreads )
    do
    {
        bytes = socket_read();

        if(bytes > 0)
        {
            readData = 1;
        }

        usleep ( 100000 ); //Prevents spin-wait from taking up too much CPU usage. (100 ms)
    }
    while ((readData == 0)||(bytes > 0)); //quit when you finally get something

    //close ( conn );
    //conn = -1;

    //close ( sock );
}

int socket_read ()
{
    char buffer[BUFFSIZE];
    int received = 0;
    int  count        = 0;
    int  actualCount  = 0;
    int  strCount     = 0;
    char tempString[1024];
    int bytes = 0;
    socklen_t echoServerAddrLength = sizeof ( struct sockaddr_in );

    bytes = recvfrom ( sock, buffer, BUFFSIZE - 1, 0, ( struct sockaddr * ) & echoserver, &echoServerAddrLength );

    //Check src here

    if ( bytes > 0 )
    {
        struct UdpCommHeader *header = buffer;
        
        if((header->status & 1) == 1)
        {
            //Ack Received
            printf("Ack from client received!\n");

/*           //Send notification of receipt of ack to server
           UdpCommHeader ackConfirmHeader;
           ackConfirmHeader.status = 1;
           sendto ( sock, &ackConfirmHeader, sizeof(ackConfirmHeader), 0,
                    ( struct sockaddr * ) &echoserver,
                    sizeof ( echoserver ) ); */
        }
        else if((header->status & 2) == 2)
        {
             received += bytes;
             buffer[bytes] = '\0';        // Assure null terminated string

             strCount = 0;
             actualCount = 0;

             for ( count = 0; count < bytes; count++ )
             {
                 if ( buffer[count] != '\0' )
                 {
                     tempString[strCount++] = buffer[count];
                     tempString[strCount]   = '\0';
                     actualCount++;
                 }
             }

             if ( actualCount != 0 )
             {
                 printf ( "%s", tempString );
                 fflush ( stdout ); //forces the stdout output from printf out. a write command would work also or setvbuf.
             }

           //Send notification of receipt of data to server
           struct UdpCommHeader ackConfirmHeader;
           ackConfirmHeader.status = 1;
           sendto ( sock, &ackConfirmHeader, sizeof(ackConfirmHeader), 0,
                    ( struct sockaddr * ) &echoserver,
                    sizeof ( echoserver ) );
           //from here a timeout and resending of ack should happen
           
        }
    }

    return bytes;
}

void *read_thread ( void *ptr )
{
    int readData = 0;
    int bytes = 0;

    //while ( runThreads )
    do
    {
        bytes = socket_read();

        if(bytes > 0)
        {
            readData = 1;
        }

        usleep ( 100000 ); //Prevents spin-wait from taking up too much CPU usage. (100 ms)
    }
    while ((readData == 0)||(bytes > 0)); //quit when you finally get something

    //printf ( "closing read_thread.......\n" );

    return NULL;
}

int main ( int argc, char *argv[] )
{
    int port;

    if ( argc != 2 )
    {
        fprintf ( stderr, "USAGE: simple-server <port>\n" );
        exit ( 1 );
    }

    port = atoi ( argv[1] );

    server_init ( port );

    while(1)
    {
        server_listen();
    }

    close(sock);


    printf ( "closing app.......\n" );

    exit ( 0 );
}
