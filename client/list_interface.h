#include "bool.h"

/* Pushes a client into the list */
void list_push(uint32_t ip, uint16_t port);
    
/* Removes a client from the list */
void list_remove(uint32_t ip, uint16_t port);

/* Returns true if client exists in the list */
bool list_find(uint32_t ip, uint16_t port);

/* Print list */
void list_print();
