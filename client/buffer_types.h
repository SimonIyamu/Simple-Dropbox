#define PATHNAME_LENGTH 128

struct buffer_tuple{
    char pathname[PATHNAME_LENGTH];
    int version; /*TODO*/
    uint32_t ip;
    uint16_t port;
};
