#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "buffer_types.h"
#include "bool.h"

/* Circular buffer */
static struct buffer_tuple *ring;

static int head,tail,max;
static bool full;

pthread_mutex_t mtx;
pthread_cond_t cond_nonempty;
pthread_cond_t cond_nonfull;

void buff_init(int bufferSize){
    /* Allocate memory for the circular buffer */
    ring = malloc(sizeof(struct buffer_tuple)*bufferSize);
    int i;
    for(i=0; i<bufferSize; i++) ring[i].version = 0;

    max = bufferSize;
    head = 0;
    tail = 0;
    full = false;

    pthread_mutex_init(&mtx, 0);
    pthread_cond_init(&cond_nonempty,0);
    pthread_cond_init(&cond_nonfull,0);
}

void buff_free(){
    printf("Freeing buffer\n");
    free(ring);
    pthread_mutex_destroy(&mtx);
    pthread_cond_destroy(&cond_nonfull);
    pthread_cond_destroy(&cond_nonempty);
}

/* State checks */
bool buff_isFull(){ return full; }
bool buff_isEmpty(){ return (!full && (head == tail));}

int buff_insert(char *pathname,int version, uint32_t ip, uint16_t port){
    pthread_mutex_lock(&mtx);

    /* Sleep until the buffer is not full */
    while(buff_isFull())
        pthread_cond_wait(&cond_nonfull,&mtx);

    printf("Inserting tuple to ring buffer...\n");

    /* Insert the new tuple */
    if(pathname != NULL)
        strcpy(ring[head].pathname,pathname);
    else
        strcpy(ring[head].pathname,"");
    ring[head].version = version;
    ring[head].ip = ip;
    ring[head].port = port;

    /* Increase pointer(s) */
    if(buff_isFull())
        tail = (tail + 1 ) % max;
    head = (head+1) % max;
    full = (head == tail);

    pthread_mutex_unlock(&mtx);
    pthread_cond_signal(&cond_nonempty);
}

struct buffer_tuple buff_remove(){
    pthread_mutex_lock(&mtx);

    /* Sleep until the buffer is not full */
    while(buff_isEmpty())
        pthread_cond_wait(&cond_nonempty,&mtx);

    struct buffer_tuple r;

    if(!buff_isEmpty()){
        if(ring[tail].version != 0)
            strcpy(r.pathname,ring[tail].pathname);
        else
            strcpy(r.pathname,"");
        r.version = ring[tail].version;
        r.ip = ring[tail].ip;
        r.port = ring[tail].port;

        /* Decrease pointer */
        tail = (tail + 1) % max;
        full = false;
    }else{
        strcpy(r.pathname,"");
        r.version = -1;
        r.ip = -1;
        r.port = -1;
    }

    pthread_mutex_unlock(&mtx);
    pthread_cond_signal(&cond_nonfull);

    printf("Removed a tuple from the ring buffer\n");

    return r;
}

void buff_print(){
    printf("--Cirular Buffer--\n");
    int i;
    for(i=0; i<max; i++){
        printf(" (%s,%d,%u,%hu)",ring[i].pathname,ring[i].version,ring[i].ip,ring[i].port);
        if(i==head)
            printf("<---head");
        if(i==tail)
            printf("<---tail");
        printf("\n");
    }
}
