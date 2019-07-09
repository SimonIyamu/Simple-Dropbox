#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "client_interface.h"
#include "buffer_interface.h"
#include "list_interface.h"

extern char *inputDir, *mirrorDir, *serverHostname;

int main(int argc, char *argv[]){
    uint16_t myPort, serverPort;
    int sock, i, workerThreads, bufferSize;
    fd_set active_fd_set, read_fd_set;
    struct sockaddr_in clientname;
    socklen_t size;

    /* Handling command line arguments */
    if(argc==15){
        int i;
        for(i=1 ; i<argc ; i++){
            if(!strcmp(argv[i],"-p"))
                myPort = atoi(argv[i+1]);
            if(!strcmp(argv[i],"-i"))
                inputDir = argv[i+1];
            if(!strcmp(argv[i],"-m"))
                mirrorDir = argv[i+1];
            if(!strcmp(argv[i],"-w"))
                workerThreads = atoi(argv[i+1]);
            if(!strcmp(argv[i],"-b"))
                bufferSize = atoi(argv[i+1]);
            if(!strcmp(argv[i],"-sp"))
                serverPort = atoi(argv[i+1]);
            if(!strcmp(argv[i],"-shm"))
                serverHostname = argv[i+1];
        }
    }
    else{
        printf("Wrong ammount of arguments.\nUsage: ./dropbox_client -i inputDir -m MirrorDir -p portNum -w workerThreads -b bufferSize -sp serverPort -shm serverHostname\n");
        return(1);
    } 

    assign_gl_var(myPort, serverPort);

    /* Signal for termination */ 
    static struct sigaction act1;
    act1.sa_handler = sighandler;
    sigemptyset(&(act1.sa_mask));
    sigaction(SIGINT, &act1, NULL);

    /* Allocate memory for the circular buffer */
    buff_init(bufferSize);

    /* Send LOG_ON message to the server. */
    send_log_on();

    /* Send GET_CLIENTS message to the server. */
    send_get_clients();

    /* Create worker threads */
    create_threads(workerThreads);

    /* Create the socket and set it up to accept connections. */
    sock = make_socket(myPort);
    if (listen (sock, 1) < 0){
        perror ("listen");
        exit (EXIT_FAILURE);
    }

    /* Initialize the set of active sockets. */
    FD_ZERO (&active_fd_set);
    FD_SET (sock, &active_fd_set);

    while (1)
    {
        /* Block until input arrives on one or more active sockets. */
        read_fd_set = active_fd_set;
        if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0){
            perror ("select");
            exit (EXIT_FAILURE);
        }

        /* Service all the sockets with input pending. */
        for (i = 0; i < FD_SETSIZE; ++i)
            if (FD_ISSET (i, &read_fd_set)){
                if (i == sock){
                    /* Connection request on original socket. */
                    int new;
                    size = sizeof (clientname);
                    new = accept (sock,
                                  (struct sockaddr *) &clientname,
                                  &size);
                    if (new < 0){
                        perror ("accept");
                        exit (EXIT_FAILURE);
                    }

                    /* Find the IP and port of the sender */
                    uint32_t client_ip = ntohl(clientname.sin_addr.s_addr);
                    uint16_t client_port = ntohs(clientname.sin_port);

                    /* Check if he is in the list of clients.
                     * If not, ignore the message */
                    //if (!list_find(client_ip,client_port)){
                    //    printf("YOUUU SHALL NOT PASS\n");
                    //    continue;
                    //}
 
                    FD_SET (new, &active_fd_set);
                }
                else{
                    /* Data arriving on an already-connected socket. */
                    if (read_message(i) < 0){
                        close (i);
                        FD_CLR (i, &active_fd_set);
                    }
                }
            }
    }

    return 0;
}
