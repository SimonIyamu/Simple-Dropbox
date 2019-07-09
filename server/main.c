#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "server_interface.h"

int main(int argc, char *argv[]){
    int sock,i;
    uint16_t portNum;
    fd_set active_fd_set, read_fd_set;
    struct sockaddr_in clientname;
    socklen_t size;

    /* Handling command line arguments */
    if(argc==3 && !strcmp(argv[1],"-p"))
            portNum = atoi(argv[2]);
    else{
        printf("Wrong arguments.");
        printf("(Expected execution: ./dropbox_server -p portNum)\n");
        return(1);
    }

    /* Signal for termination */ 
    static struct sigaction act1;
    act1.sa_handler = sighandler;
    sigemptyset(&(act1.sa_mask));
    sigaction(SIGINT, &act1, NULL);

    /* Create the socket and set it up to accept connections. */
    sock = make_socket(portNum);
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
                    FD_SET (new, &active_fd_set);
                }
                else{
                    /* Data arriving on an already-connected socket. */
                    if (read_message (i) < 0){
                        close (i);
                        FD_CLR (i, &active_fd_set);
                    }
                }
            }
    }
}

