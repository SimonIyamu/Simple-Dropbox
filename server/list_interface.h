#include "list_types.h"

/* Pushes a client into the list */
void list_push(uint32_t ip, uint16_t port);
    
/* Removes a client from the list */
void list_remove(uint32_t ip, uint16_t port);

/* Returns true if client exists in the list */
bool list_find(uint32_t ip, uint16_t port);

/* Print list */
void list_print();

/* Send USER_ON/OFF to every other client, to inform them that this client
 * has appeared/left. */
void list_broadcast(uint32_t ip, uint16_t port, bool on);

/* Send client list to the client of this socket */
void send_client_list(int sock);
