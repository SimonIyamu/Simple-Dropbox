#include "bool.h"

/* Send LOG_ON message to the server */
void send_log_on ();

/* Send LOG_OFF message to the server */
void send_log_off();

/* Send GET_CLIENTS message to the server and read the answer*/
void send_get_clients();

/* Sends GET_FILE request */
int send_get_file(uint32_t ip, uint16_t port, const char *fileRPath, time_t version);

/* Sends GET_FILE_LIST request */
int send_get_file_list(uint32_t ip, uint16_t port);

/* Send the names of the files that this client will share */
void send_file_list(int sock,const char *inputDirPath, const char *rPath);

int read_message(int filedes);

int create_threads(int n);

void assign_gl_var(uint16_t p, uint16_t sp);

/* Checks if the given path is a directory */
bool isDir(const char *path);

/* Concatinate two paths */
char *pathcat(const char *a,const char *b);

int make_socket(uint16_t port);

/* Returns this host's IP address as a string */
char *myIP();

/* hton for 64bit variables */
uint64_t htonll(const uint64_t *input);
uint64_t ntohll(const uint64_t *input);

void init_sockaddr(struct sockaddr_in *name,const char *hostname,uint16_t port);

int init_sockaddr_from_IP(struct sockaddr_in *name, uint32_t ip, uint16_t port);

/* Signal handlers */
void threadTerminator(int);
void sighandler(int);
