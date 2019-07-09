#include "buffer_types.h"

/* Allocate memory for the circluar buffer */
void buff_init(int bufferSize);

void buff_free();

bool buff_isFull();

bool buff_isEmpty();

/* Inser tuple to the head of the buffer */
int buff_insert(char *pathname,int version, uint32_t ip, uint16_t port);

/* Pop tail */
struct buffer_tuple buff_remove();

void buffer_print();
