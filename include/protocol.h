#ifndef HEADER_PROTOCOL
#define HEADER_PROTOCOL

#include <netinet/ip.h>

#include "encoding.h"

enum mc_packet_status {
    PKT_INCOMPLETE  = 1 << 0,
    PKT_COMPLETE    = 1 << 1,
    PKT_TOO_LARGE   = 1 << 2,
    PKT_INVALID     = 1 << 3,
    PKT_ALLOCATED   = 1 << 4
};

typedef struct mc_packet {
    int size;
    int id;

    uint32_t received;
    size_t read_pos;
    uint8_t *data;

    int status;
} mc_packet;

enum protocol_state {
    STATE_PRE_HANDSHAKE,
    STATE_PRE_REQUEST,
    STATE_PRE_PING
};

typedef struct mc_client {
    int fd;
    struct sockaddr_in addr;

    mc_packet *packet;
    int state;
} mc_client;

int init_packet(mc_packet *packet, uint8_t *buffer, size_t buffer_size, size_t *offset);
void cleanup_packet(mc_packet *packet);
int read_packet(mc_packet *packet, uint8_t *buffer, size_t buffer_size, size_t *offset);

int read_packet_var_int(mc_packet *packet, int *target);
int read_packet_short(mc_packet *packet, uint16_t *target);
int read_packet_string(mc_packet *packet, char *target, size_t target_length);

#endif

