typedef enum { false, true } bool;

typedef struct ListNodeTag{
    /* Client info */
    uint32_t ip; /* inet_pton of an IP */
    uint16_t port; /* Listening port */

    struct ListNodeTag *next;
} ListNode;
