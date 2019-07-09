#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "list_types.h"
#include "server_interface.h"

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

/* Send USER_ON/OFF to every other client, to inform them that this client 
 * has appeared/left. */
void list_broadcast(uint32_t ip,uint16_t port,bool on){
    /* Iterate over the clients on the list */
    ListNode *temp = head;
    while(temp!=NULL){
        //printf("1. %d != %d && %d != %d\n",temp->ip,ip,temp->port,port);
        /* If its not the new client */
        if(temp->ip != ip || temp->port != port){
            if(on){
                send_user_on(temp->ip,temp->port,ip,port);
            }else{
                send_user_off(temp->ip,temp->port,ip,port);
            }
        }
        temp=temp->next;
    }
}


/* Send client list to the client of this socket */
void send_client_list(int sock){
    int nbytes;
    char message[] = "CLIENT_LIST";
    int n = list_len();

    /* Conver to network byte order */
    n = htonl(n);

    /* Send */
    write(sock, message, strlen(message) + 1);
    write(sock, &n, sizeof(int));

    /* Send clients one by one */
    ListNode *temp = head;
    while(temp!=NULL){
        /* This while will loop list_len() times, and the client knows it */
        uint32_t n_ip = htonl(temp->ip);
        uint16_t n_port = htons(temp->port);
        write(sock, &n_ip, 4);
        write(sock, &n_port, 2);
        /*sprintf(message,"<%u,%hu>",htonl(temp->ip),htons(temp->port));
        nbytes = write(sock , message, strlen(message) + 1);
        if (nbytes < 0){
            printf("Write fail\n");
            exit (EXIT_FAILURE);
        }*/

        temp=temp->next;
    }

}
