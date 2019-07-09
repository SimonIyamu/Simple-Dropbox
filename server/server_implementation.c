#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "server_interface.h"
#include "list_interface.h"

#define MAXMSG  512
#define MAX_UINT32_T_LEN 10
#define MAX_UINT16_T_LEN 5

int read_message(int sock){
    char request[MAXMSG];
    int nbytes;

    /* Read first token */
    int ch='a',i=0, r;
    do{
        r = read(sock,&ch,1);
        if (r <= 0) return -1;
        request[i]=ch;
        i++;
    }while(ch != '\0');

    /* Read the first token of the buffer. */
    if (!strcmp(request,"LOG_ON")){
        /* Retrieve info */
        uint32_t n_ip;
        uint16_t n_port;
        read(sock, &n_ip, sizeof(uint32_t));
        read(sock, &n_port, sizeof(uint16_t));

        /* Convert them to host byte order */
        uint32_t ip = ntohl(n_ip);
        uint16_t portNum = ntohs(n_port);

        /* Convert the ip number to printable string */
        char ip_string[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &ip, ip_string, INET_ADDRSTRLEN);

        printf("LOG_ON request from client with IP: %s and Port: %hu.\n",ip_string, portNum);

        /* If it doesn't already exist in the list, add it */
        if(!list_find(ip,portNum))
            list_push(ip,portNum);

        list_print();
        /* Notify every other client that this new client has appeared */
        list_broadcast(ip,portNum,true);
    }
    else if (!strcmp(request,"GET_CLIENTS"))
        send_client_list(sock);
    else if (!strcmp(request,"LOG_OFF")){
        /* Retrieve info */
        uint32_t n_ip;
        uint16_t n_port;
        read(sock, &n_ip, sizeof(uint32_t));
        read(sock, &n_port, sizeof(uint16_t));

        /* Convert them to host byte order */
        uint32_t ip = ntohl(n_ip);
        uint16_t portNum = ntohs(n_port);

        /* Convert the ip number to printable string */
        char ip_string[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &ip, ip_string, INET_ADDRSTRLEN);

        printf("LOG_OFF request from client with IP: %s and Port: %hu.\n",ip_string, portNum);

        /* If it doesn't exist in the list, answer with error message */
        if(!list_find(ip,portNum))
            write(sock, "ERROR_IP_PORT_NOT_FOUND_IN_LIST", MAXMSG);
        else
            list_remove(ip,portNum);

        list_print();
        /* Notify every other client that this new client has appeared */
        list_broadcast(ip,portNum,false);
    }else
        printf("Received Unknown request\n");
    return 0;
}

int init_sockaddr_from_IP(struct sockaddr_in *name, uint32_t ip, uint16_t port){
    name->sin_family = AF_INET;
    name->sin_port = htons (port);
    name->sin_addr.s_addr = ip;
}

/* Send a string in the form: USER_ON <IP address, portNum> 
 * in order to inform a client that a new client with ip2 and port2 has appeared. */
int send_user_on(uint32_t ip, uint16_t port,uint32_t ip2, uint16_t port2){
    int sock, nbytes;
    struct sockaddr_in clientaddr;
    char message[] = "USER_ON";

    /* Convert IP number and port to network byte order */
    uint32_t n_ip= htonl(ip);
    uint16_t n_port= htons(port);

    /* Create the socket. */
    sock = socket (AF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        perror ("socket (client)");
        exit (EXIT_FAILURE);
    }

    /* Connect to the server. */
    init_sockaddr_from_IP(&clientaddr, ip, port);
    if (connect (sock, (struct sockaddr *) &clientaddr,sizeof (clientaddr)) < 0){
        perror ("connect (server)");
        exit (EXIT_FAILURE);
    }

    /* Send the message */
    nbytes = write (sock, message, strlen (message) + 1);
    if (nbytes < 0){
        printf("Write faile\n");
        exit (EXIT_FAILURE);
    }

    /* Send ip and port */
    uint32_t n_ip2= htonl(ip2);
    uint16_t n_port2= htons(port2);
    write(sock, &n_ip2, sizeof(uint32_t));
    write(sock, &n_port2, sizeof(uint16_t));

    close (sock);
}

/* Send a string in the form: USER_OFF <IP address, portNum>
 * in order to inform a client that a client with ip2 and port2 has left. */
int send_user_off(uint32_t ip, uint16_t port, uint32_t ip2, uint16_t port2){
    int sock, nbytes;
    struct sockaddr_in clientaddr;
    char message[] = "USER_OFF";

    /* Convert IP number and port to network byte order */
    uint32_t n_ip= htonl(ip);
    uint16_t n_port= htons(port);

    /* Create the socket. */
    sock = socket (AF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        perror ("socket (client)");
        exit (EXIT_FAILURE);
    }

    /* Connect to the server. */
    init_sockaddr_from_IP(&clientaddr, ip, port);
    if (connect (sock, (struct sockaddr *) &clientaddr,sizeof (clientaddr)) < 0){
        perror ("connect (server)");
        exit (EXIT_FAILURE);
    }

    /* Send the message */
    nbytes = write (sock, message, strlen (message) + 1);
    if (nbytes < 0){
        printf("Write faile\n");
        exit (EXIT_FAILURE);
    }

    /* Send ip and port */
    uint32_t n_ip2= htonl(ip2);
    uint16_t n_port2= htons(port2);
    write(sock, &n_ip2, sizeof(uint32_t));
    write(sock, &n_port2, sizeof(uint16_t));

    close (sock);

}

/* Returns the IP of the current hostname in a ASCII string */
char *myIP(){
    char hostbuffer[256]; 
    char *IPbuffer; 
    struct hostent *host_entry; 
    int hostname; 

    gethostname(hostbuffer, sizeof(hostbuffer)); 
    host_entry = gethostbyname(hostbuffer); 
    IPbuffer = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0])); 

    return IPbuffer;
}

int make_socket (uint16_t port){
    int sock;
    struct sockaddr_in name;

    /* Create the socket. */
    sock = socket (AF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        perror ("socket");
        exit (EXIT_FAILURE);
    }

    /* Give the socket a name. */
    name.sin_family = AF_INET;
    name.sin_port = htons (port);
    name.sin_addr.s_addr = htonl (INADDR_ANY);
    if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0){
        perror ("bind");
        exit (EXIT_FAILURE);
    }

    return sock;
}

void sighandler(int signum){
    printf("Server terminating.\n");
}
