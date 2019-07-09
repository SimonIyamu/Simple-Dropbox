int read_message(int filedes);

/* Send USER_ON message to certain IP and port(about client of ip2 and port2) */
int send_user_on(uint32_t ip, uint16_t port, uint32_t ip2, uint16_t port2);

/* Send USER_OFF message to certain IP and port(about client of ip2 and port2) */
int send_user_off(uint32_t ip, uint16_t port, uint32_t ip2, uint16_t port2);

int make_socket (uint16_t port);

/* Returns this host's IP address as a string */
char *myIP();

void sighandler(int);
