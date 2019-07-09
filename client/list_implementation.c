#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "list_types.h"
#include "bool.h"
#include "client_interface.h"

#define MAXMSG 512

ListNode *head = NULL;

/* Pushes a client into the list */
void list_push(uint32_t ip, uint16_t port){
    ListNode *newNode = (ListNode *)malloc(sizeof(ListNode));
    newNode->ip=ip;
    newNode->port=port;
    newNode->next = NULL;

    if(head == NULL)
        head = newNode;
    else{
        ListNode *temp = head;
        while(temp->next!=NULL){
            temp=temp->next;
        }
        temp->next = newNode;
    }
}

/* Removes a client from the list */
void list_remove(uint32_t ip, uint16_t port){
    ListNode *current = head;
    ListNode *previous= NULL;

    if(head==NULL)
        return;

    while(current->ip != ip || current->port != port){
        if(current->next==NULL)
            return;
        else{
            previous = current;
            current = current->next;
        }
    }

    if(current == head)
        head = head->next;
    else
        previous->next = current->next;
    free(current);
}

/* Returns true if client exists in the list */
bool list_find(uint32_t ip, uint16_t port){
    if(head==NULL)
        return false;    

    ListNode *temp = head;
    while(temp!=NULL){
        if(temp->ip==ip && temp->port==port)
            return true;
        temp=temp->next;
    }
    /* Not found*/
    return false;
}

/* Print list */
void list_print(){
    ListNode *temp = head;
    while(temp!=NULL){
        /* Convert ip from uint32_t to printable string */
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &temp->ip, ip, INET_ADDRSTRLEN);

        printf("IP:%s Port:%d\n",ip,temp->port);

        temp=temp->next;
    }
}

/* Returns the length of the list */
int list_len(){
    int count = 0;
    ListNode *temp = head;
    while(temp!=NULL){
        count++;
        temp=temp->next;
    }
    return count;
}
