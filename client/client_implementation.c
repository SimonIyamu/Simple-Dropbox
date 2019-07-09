#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>

#include "client_interface.h"
#include "buffer_interface.h"
#include "list_interface.h"

#define MAXMSG 512
#define BUFFERSIZE 10

uint16_t myPort;
uint16_t serverPort;
char *inputDir, *mirrorDir, *serverHostname;
pthread_t *tids;
int workerThreads=0;

/* Send LOG_ON message to the server */
void send_log_on(){
    int sock, nbytes;
    struct sockaddr_in servername;
    char *ipstring = myIP();

    /* Convert IP from printable to number (stored in ip_addr) */
    uint32_t ip;
    inet_pton(AF_INET, ipstring, &ip);

    char message[] = "LOG_ON";
    /* Convert IP number and port to network byte order */
    uint32_t n_ip= htonl(ip);
    uint16_t n_port= htons(myPort);

    /* Form the message in a string */    
    //sprintf(message,"LOG_ON <%u,%hu>",n_ip,n_port);

    /* Create the socket. */
    sock = socket (AF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        perror ("socket (client)");
        exit (EXIT_FAILURE);
    }

    /* Connect to the server.*/
    init_sockaddr (&servername, serverHostname, serverPort);
    if (connect (sock, (struct sockaddr *) &servername, sizeof (servername)) < 0){
        perror ("connect");
        exit (EXIT_FAILURE);
    }

    /* Send the message */
    nbytes = write (sock, message, strlen (message) + 1);
    if (nbytes < 0){
        printf("Write faile\n");
        exit (EXIT_FAILURE);
    }

    write(sock, &n_ip, sizeof(uint32_t));
    write(sock, &n_port, sizeof(uint16_t));

    close (sock);
}

/* Send LOG_OFF message to the server */
void send_log_off(){
    int sock, nbytes;
    struct sockaddr_in servername;
    char *ipstring = myIP();

    /* Convert IP from printable to number (stored in ip_addr) */
    uint32_t ip;
    inet_pton(AF_INET, ipstring, &ip);

    char message[] = "LOG_OFF";
    /* Convert IP number and port to network byte order */
    uint32_t n_ip= htonl(ip);
    uint16_t n_port= htons(myPort);

    /* Form the message in a string */    
    //sprintf(message,"LOG_ON <%u,%hu>",n_ip,n_port);

    /* Create the socket. */
    sock = socket (AF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        perror ("socket (client)");
        exit (EXIT_FAILURE);
    }

    /* Connect to the server.*/
    init_sockaddr (&servername, serverHostname, serverPort);
    if (connect (sock, (struct sockaddr *) &servername, sizeof (servername)) < 0){
        perror ("connect");
        exit (EXIT_FAILURE);
    }

    /* Send the message */
    nbytes = write (sock, message, strlen (message) + 1);
    if (nbytes < 0){
        printf("Write faile\n");
        exit (EXIT_FAILURE);
    }

    write(sock, &n_ip, sizeof(uint32_t));
    write(sock, &n_port, sizeof(uint16_t));

    close (sock);
}

/* Send GET_CLIENTS message to the server and read the answer*/
void send_get_clients(){
    printf("Sending GET_CLIENTS request...\n");
    int sock, nbytes, r;
    struct sockaddr_in servername;

    /* Create the socket. */
    sock = socket (AF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        perror ("socket (client)");
        exit (EXIT_FAILURE);
    }

    /* Connect to the server.*/
    init_sockaddr (&servername, serverHostname, serverPort);
    if (connect (sock, (struct sockaddr *) &servername, sizeof (servername)) < 0){
        perror ("connect");
        exit (EXIT_FAILURE);
    }

    /* Send the message */
    nbytes = write (sock, "GET_CLIENTS", 11 + 1);
    if (nbytes < 0){
        printf("Write faile\n");
        exit (EXIT_FAILURE);
    }

    /* Read clients list */
    char buffer[12];

    nbytes = read (sock, buffer, 12); 
    if (nbytes < 0){
        printf("Write faile\n");
        exit (EXIT_FAILURE);
    }

    /* We expect that the server will write CLIENT_LIST */
    if (!strcmp(buffer,"CLIENT_LIST")){
        int n,i;

        /* Read n and conver it to host byte order */
        read(sock, &n, sizeof(int));
        n = ntohl(n);

        /* Find my IP */
        uint32_t my_ip;
        char *ipstring = myIP();
        inet_pton(AF_INET, ipstring, &my_ip);

        /* Read n clients */
        for (i=0; i<n; i++){
            uint32_t n_ip;
            uint16_t n_port;

            /* Read ip and port */
            read(sock, &n_ip, sizeof(uint32_t));
            read(sock, &n_port, sizeof(uint16_t));

            /* Convert them to host byte order */
            uint32_t ip = ntohl(n_ip);
            uint16_t portNum = ntohs(n_port);

            /* If its this client, skip him */
            if (ip==my_ip && portNum==myPort)
                continue;

            /* Convert the ip number to printable string */
            char ip_string[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &ip, ip_string, INET_ADDRSTRLEN);

            /* If it doesn't already exist in the list, add it */
            if(!list_find(ip,portNum))
                list_push(ip,portNum);

            /* Create a directory for this client in the mirrorDir.
             * The name of this directory will be (ip)_(port) */
            char dirname[17], *dirPath;
            sprintf(dirname,"%u_%hu",ip,portNum);
            dirPath = pathcat(mirrorDir,dirname);
            if(!isDir(dirPath)){
                r = mkdir(dirPath,0777);
                if (r < 0 && errno != EEXIST){
                    perror("mkdir");
                    exit(EXIT_FAILURE);
                }
            }
            free(dirPath);

            /* Add the corresponding tuple to the buffer */
            buff_insert(NULL,0,ip,portNum);
        }
    }
    else{
            printf("Error: Unexpected response from server (%s)\n",buffer);
    }

    printf("List of other clients:\n");
    list_print();
    close (sock);
}

/* Sends GET_FILE request */
int send_get_file(uint32_t ip, uint16_t port, const char *fileRPath, time_t old_version){
    printf("Sending GET_FILE request for %s...\n",fileRPath);
    int sock, nbytes;
    struct sockaddr_in clientaddr;

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

    char message[] = "GET_FILE";

    /* Send message */
    nbytes = write(sock, message, strlen(message) + 1);
    if (nbytes < 0){
        printf("Write faile\n");
        exit (EXIT_FAILURE);
    }

    /* Send version (in network byte order) and file relative path */
    old_version = htonll(&old_version);
    write(sock, &old_version, sizeof(time_t));
    write(sock, fileRPath, strlen(fileRPath) + 1);

    /* Read first token */
    char buffer[MAXMSG];
    int ch='a',i=0,r;
    do{
        read(sock,&ch,1);
        buffer[i]=ch;
        i++;
    }while(ch != '\0');

    if(!strcmp(buffer,"FILE_NOT_FOUND"))
        printf("The file I asked to get was not found.\n");
    else if(!strcmp(buffer,"FILE_UP_TO_DATE"))
        printf("My file is up to date.\n");
    else if(!strcmp(buffer,"FILE_SIZE")){
        /* Read version and convert it to host byte order */
        time_t version;
        read(sock,&version, sizeof(time_t));
        version = ntohll(&version);

        /* Read file size and convert it to host byte order */
        uint32_t fileSize;
        read(sock,&fileSize, sizeof(uint32_t));
        fileSize = ntohl(fileSize);

        /* Read the file and write it into the mirror dir */
        printf("Receiving file %s...\n",fileRPath);
        char dirname[17], *dirPath, *mirrorFilePath;

        /* Form mirrorFilePath */
        sprintf(dirname,"%u_%hu",ip,port);
        dirPath = pathcat(mirrorDir,dirname);
        mirrorFilePath = pathcat(dirPath,fileRPath);
        free(dirPath);

        int fd = open(mirrorFilePath, O_WRONLY | O_CREAT, 0644);
        if (fd == -1){
            perror("open");
            printf(" Error on file %s\n",mirrorFilePath);
            exit(1);
        }

        int i;
        size_t items;
        int n = (int) ceil((double)fileSize / MAXMSG);

        /* for i < n-1 so that the last buffer is excluded */
        for (i=0; i<n-1; i++){
            /* Read data */
            items = read(sock, buffer, MAXMSG);

            /* Write to file */
            write(fd,buffer,items);
        }

        /* The last buffer might have less than buffSize bytes */
        if (n){ /* If file is not empty */
            if (fileSize % MAXMSG == 0)
                items = read(sock, buffer, MAXMSG);
            else
                items = read(sock, buffer, fileSize % MAXMSG);
            /* Write to file */
            write(fd,buffer,items);
        }

        /* Change the modification time */
        struct utimbuf times;
        times.modtime = times.actime = version;
        utime(mirrorFilePath, &times);

        close(fd);
        free(mirrorFilePath);

        printf("File %s transfer is complete.\n",fileRPath);
    }
}

/* Sends GET_FILE_LIST and add every file to the buffer */
int send_get_file_list(uint32_t ip, uint16_t port){
    printf("Sending GET_FILE_LIST request\n");
    int sock, nbytes;
    struct sockaddr_in clientaddr;

    /* Create the socket. */
    sock = socket (AF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        perror ("socket (client)");
        exit (EXIT_FAILURE);
    }

    /* Connect to the server. */
    usleep(500);
    init_sockaddr_from_IP(&clientaddr, ip, port);
    if (connect (sock, (struct sockaddr *) &clientaddr,sizeof (clientaddr)) < 0){
        perror ("connect (server)");
        printf(" to %hu \n",port);
        exit (EXIT_FAILURE);
    }

    char message[] = "GET_FILE_LIST";

    /* Send GET_FILE_LIST message, in order to receive the file names 
     * of the foreign client */
    nbytes = write(sock, message, strlen(message) + 1);
    if (nbytes < 0){
        printf("Write faile\n");
        exit (EXIT_FAILURE);
    }

    /* Read the answers */

    /* Read the file name length and convert it to host byte order */
    uint16_t fileNameLength;
    read(sock, &fileNameLength, 2);
    fileNameLength = ntohs(fileNameLength);
    while(fileNameLength){

        /* Read the file name */
        char *fileRPath = malloc(sizeof(char)*(fileNameLength+1));
        read(sock, fileRPath, fileNameLength + 1);

        /* Find its modification time (to determine the version) */
        char *filePath = pathcat(inputDir,fileRPath);
        struct stat filestat;
        stat(filePath,&filestat);
        time_t version = filestat.st_mtime;

        /* Add the corresponding tuple to the buffer */
        buff_insert(fileRPath,version,ip,port);

        free(filePath);
        free(fileRPath);

        /* Read the file name length and convert it to host byte order */
        read(sock, &fileNameLength, 2);
        fileNameLength = ntohs(fileNameLength);
    }
    printf("GET_FILE_LIST is complete.\n");
    close (sock);

}

void send_file_list(int sock,const char *inputDirPath, const char *rPath){
    /* rPath is the path of the directory, relative to the input directory.
     * e.x. for a directory with path user1/inputDir/aa/bb/ the inputDirPath 
     * will be user1/inputDir and rPath will be aa/bb/ */
    char *dirPath = pathcat(inputDirPath,rPath);

    struct dirent *currentFile;
    DIR *dir = opendir(dirPath);
    uint16_t fileNameLength, n_fileNameLength;
    /* For every file... */
    while((currentFile = readdir(dir)) != NULL){

        /* Ignore . and .. directories */
        if (!strcmp(currentFile->d_name,".") || !strcmp(currentFile->d_name,".."))
                continue;

        /* fileRPath is relative to the inputDir. e.x. for a file with the 
         * path user1/inputDir/newDir/foo.txt then fileRPath will be 
         * newDir/foo.txt */
        char *fileRPath = malloc(sizeof(char)*(strlen(rPath) + strlen(currentFile->d_name)+3));
        strcpy(fileRPath,rPath);
        strcat(fileRPath,currentFile->d_name);

        char *filePath = pathcat(dirPath,currentFile->d_name);

        if (isDir(filePath)){
            /* Put / in the end of the name to indicate its a directory */
            int x = strlen(fileRPath);
            fileRPath[x] = '/';
            fileRPath[x+1] = '\0';
        }

        /* Write the file name length (in network byte order) */
        fileNameLength = strlen(fileRPath);
        n_fileNameLength = htons(fileNameLength);
        write(sock, &n_fileNameLength, 2);

        /* Write the file name */
        write(sock, fileRPath, fileNameLength + 1);

        if (isDir(filePath)){
            send_file_list(sock,inputDirPath, fileRPath);
            continue;
        }

        free(filePath);
        free(fileRPath);
    }
    closedir(dir);
    free(dirPath);
}

int read_message(int sock){

    /* Read first token */
    char request[MAXMSG];
    int ch='a',i=0, r;
    do{
        r = read(sock,&ch,1);
        if (r<=0) return -1;
        request[i]=ch;
        i++;
    }while(ch != '\0');

    /* Read the first token of the bufer. */
    if (!strcmp(request,"USER_ON")){
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

        printf("USER_ON request from client with IP: %s and Port: %hu.\n",ip_string, portNum);

        /* If it doesn't already exist in the list, add it */
        if(!list_find(ip,portNum))
            list_push(ip,portNum);

        /* Create a directory for this client in the mirrorDir.
         * The name of this directory will be (ip)_(port) */
        char dirname[17], *dirPath;
        sprintf(dirname,"%u_%hu",ip,portNum);
        dirPath = pathcat(mirrorDir,dirname);
        if(!isDir(dirPath)){
            r = mkdir(dirPath,0777);
            if (r < 0 && errno != EEXIST){
                perror("mkdir");
                exit(EXIT_FAILURE);
            }
        }
        free(dirPath);


        /* Add the corresponding tuple to the buffer */
        buff_insert(NULL,0,ip,portNum);
    }
    else if (!strcmp(request,"GET_FILE_LIST")){
        printf("Sending file list...\n");
        send_file_list(sock,inputDir,"");
        printf("File list has been sent.\n");

        /* Write 0 to terminate the transfer 
         * (hton is unnecessary because its 0) */
        uint16_t zero = 0;
        write(sock, &zero, 2);
    }
    else if (!strcmp(request,"GET_FILE")){
        /* Read version and convert it to host byte order */
        time_t version;
        read(sock, &version, sizeof(time_t));
        version = ntohll(&version);
        
        char fileRPath[MAXMSG];
        read(sock, fileRPath, MAXMSG);

        /* filePath will be inputDirPath + fileRPath */
        char *filePath = pathcat(inputDir,fileRPath);
        FILE *fp = fopen(filePath,"r");
        if(fp == NULL){
            if (errno == ENOENT)
                write(sock,"FILE_NOT_FOUND",14+1);
        }else{

            /* Find the version of the original file (modification time) */
            struct stat filestat;
            stat(filePath,&filestat);
            time_t new_version = filestat.st_mtime;

            /* If this is the first time sending the file (version == -1) or
             * the file is updated */
            if (version == -1 || version != new_version){

                /* Find the file size */
                fseek(fp, 0L, SEEK_END);
                uint32_t fileSize = ftell(fp);
                rewind(fp);

                /* Write request title */
                write(sock,"FILE_SIZE",9+1); 

                /* Write version in network byte order */
                time_t n_new_version = htonll(&new_version);
                write(sock,&n_new_version, sizeof(time_t));

                /* Write the file size in network byte order */
                uint32_t n_fileSize = htonl(fileSize);
                write(sock,&n_fileSize, sizeof(uint32_t));

                /* Write the file itself */
                printf("Sending file %s...\n",fileRPath);
                int i;
                int n = (int) ceil((double)fileSize / MAXMSG); /* number of loops */
                char buffer[MAXMSG];
                size_t items;
                for (i=0; i<n; i++){
                    items = fread(buffer, sizeof(char), MAXMSG, fp);
                    if (items == -1){
                        printf("Error in fread\n");
                        exit(1);
                    }
                    write(sock, buffer, items);
                }

                printf("Sending of file %s was complete.\n",fileRPath);
            }
            else{
                /* version == new_version so the file is up to date.
                 * Send a corresponding answer */
                write(sock,"FILE_UP_TO_DATE",15+1);
            }
        }
        free(filePath);
        fclose(fp);
    }
    else if (!strcmp(request,"USER_OFF")){
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

        printf("USER_OFF request from client with IP: %s and Port: %hu.\n",ip_string, portNum);

        /* Remove him from the list */
        list_remove(ip,portNum);
    }else
        printf("Received Unknown request(%s)\n",request);
    return 0;
}

/* Threads function */
void *worker(void *argp){
    printf("Thread %lu was created\n",pthread_self());

    /* Detach thread */
    int err;
    if (err = pthread_detach(pthread_self())){
        fprintf(stderr, "pthread_create error: %s\n", strerror(err));
        exit(1);
    }

    /* Signal for termination */ 
    static struct sigaction act1;
    act1.sa_handler = threadTerminator;
    sigemptyset(&(act1.sa_mask));
    sigaction(SIGUSR2, &act1, NULL);

    while(1){
        /* Get an item from the buffer (or wait) */
        struct buffer_tuple tup = buff_remove();

         /* There are two kinds of tuples, and we distinguish them by looking
         * at the version. 
         * If version is 0 then the thread will establish a connection to the
         * client of ip and port and it will send a GET_FILE_LIST request.
         * Otherwise it will send a GET_FILE request for the pathname's 
         * file. */
        if(tup.version == 0){
            printf(":::Category 1\n");
            send_get_file_list(tup.ip, tup.port);
        }else{
            /* Check if the client is registered the list */
            if(!list_find(tup.ip,tup.port)){
                printf("Aborted: Client not in list\n");
                continue;
            }

            /* Form the mirrorFilePath */
            char dirname[17], *dirPath, *mirrorFilePath;
            sprintf(dirname,"%u_%hu",tup.ip,tup.port);
            dirPath = pathcat(mirrorDir,dirname);
            mirrorFilePath = pathcat(dirPath,tup.pathname);
            free(dirPath);

            /* If the last char is / this means its a directory. So mkdir */
            if(tup.pathname[strlen(tup.pathname)-1] == '/'){
                int r;
                if ((r = mkdir(mirrorFilePath,0777)) < 0  && errno != EEXIST){
                    perror("mkdir");
                    printf(" %s\n",mirrorFilePath);
                    exit(EXIT_FAILURE);
                }
            }
            else{
                /* Its a file */

                /* Check whether the file exists or not */
                if (access(mirrorFilePath, F_OK) == -1){
                    /* If it doesnt */

                    /* version=-1 represents that the file didnt already exist */
                    send_get_file(tup.ip, tup.port, tup.pathname, -1);
                }
                else{
                    /* If does */
                    send_get_file(tup.ip, tup.port, tup.pathname, tup.version);
                }
            }
            free(mirrorFilePath);
        }
    }

}

/* Self explanatory */
int create_threads(int n){
    int i,err;
    workerThreads=n;

    /* tids is an array of all the thread ids */
    tids = malloc(sizeof(pthread_t)*n);

    for(i=0; i<n; i++){
        /* Create the thread */
        if (err = pthread_create(&(tids[i]), NULL, worker,NULL)){
            fprintf(stderr, "pthread_create error: %s\n", strerror(err));
            exit(1);
        }
    }
}

void assign_gl_var(uint16_t p, uint16_t sp){
    myPort=p;
    serverPort=sp;
}

/* Concatinate two paths */
char *pathcat(const char *a,const char *b){
    char *path = (char*) malloc(sizeof(char)*(strlen(a) + strlen(b) + 2));
    strcpy(path,a);
    strcat(path,"/");
    strcat(path,b);
    return path;
}

/* Checks if the given path is a directory */
bool isDir(const char *path) {
   struct stat statbuf;

   if (stat(path, &statbuf) != 0){
       /* File does not even exist */
       return false;
   }

   return S_ISDIR(statbuf.st_mode);
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

uint64_t ntohll(const uint64_t *input){
    uint64_t rval;
    uint8_t *data = (uint8_t *)&rval;

    data[0] = *input >> 56;
    data[1] = *input >> 48;
    data[2] = *input >> 40;
    data[3] = *input >> 32;
    data[4] = *input >> 24;
    data[5] = *input >> 16;
    data[6] = *input >> 8;
    data[7] = *input >> 0;

    return rval;
}

uint64_t htonll(const uint64_t *input){
    return (ntohll(input));
}



void init_sockaddr (struct sockaddr_in *name, const char *hostname, uint16_t port){
    struct hostent *hostinfo;

    name->sin_family = AF_INET;
    name->sin_port = htons (port);
    hostinfo = gethostbyname (hostname);
    if (hostinfo == NULL){
        printf ("Unknown host %s.\n", hostname);
        exit (EXIT_FAILURE);
    }
    name->sin_addr = *(struct in_addr *) hostinfo->h_addr;
}

int init_sockaddr_from_IP(struct sockaddr_in *name, uint32_t ip, uint16_t port){
    name->sin_family = AF_INET;
    name->sin_port = htons (port);
    name->sin_addr.s_addr = ip;
}

void threadTerminator(int signum){
    printf("Thread %lu is exiting.\n",pthread_self());
    pthread_exit(NULL);
}

void sighandler(int signum){
    /* Terminate threads */
    int i;
    for(i=0; i<workerThreads; i++)
        pthread_kill(tids[i],SIGUSR2);

    /* Send log off to the server */
    send_log_off();
    buff_free();

    free(tids);
    exit(0);
}
