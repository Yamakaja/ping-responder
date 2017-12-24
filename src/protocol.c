#include <string.h>
#include <stdlib.h>

#include "protocol.h"

int init_packet(mc_packet *packet, uint8_t *buffer, size_t buffer_size, size_t *offset) {
    packet->status = 0;
    packet->received = 0;
    packet->data = NULL;
    packet->read_pos = 0;

    if (read_var_int(buffer, buffer_size, offset, &packet->size)) {
        packet->status = PKT_INVALID;
        return -1;
    }

    if (packet->size > 1 << 15) {
        packet->status = PKT_TOO_LARGE;
        return -1;
    }

    if (packet->size < 1) {
        packet->status = PKT_INVALID;
        return -1; 
    }

    if (read_var_int(buffer, buffer_size, offset, &packet->id)) {
        packet->status = PKT_INVALID;
        return -1;
    }

    packet->size -= required_var_int_bytes(packet->id);

    packet->data = calloc(1, packet->size);
    if (packet->data == NULL) {
        packet->status = PKT_INVALID;
        return -1;
    }
    packet->status |= PKT_ALLOCATED | PKT_INCOMPLETE;

    return 0;
}

void cleanup_packet(mc_packet *packet) {
    if (packet->status & PKT_ALLOCATED)
        free(packet->data);
}

int read_packet(mc_packet *packet, uint8_t *buffer, size_t buffer_size, size_t *offset) {
    if (packet->status & PKT_COMPLETE)
        return 0;

    if (!(packet->status & PKT_INCOMPLETE) || !(packet->status & PKT_ALLOCATED))
        return -1;

    size_t to_read = packet->size - packet->received;
    size_t readable = buffer_size - *offset;

    to_read = to_read > readable ? readable : to_read;

    memcpy(packet->data + packet->received, buffer + *offset, to_read);
    packet->received += to_read;

    if (packet->received == packet->size) {
        packet->status &= ~PKT_INCOMPLETE;
        packet->status |= PKT_COMPLETE;
    }

    *offset += to_read;

    return 0;
}

int read_packet_var_int(mc_packet *packet, int *target) {
    return read_var_int(packet->data, packet->size, &packet->read_pos, target);
}

int read_packet_short(mc_packet *packet, uint16_t *target) {
    return read_short(packet->data, packet->size, &packet->read_pos, target);
}

int read_packet_string(mc_packet *packet, char *target, size_t target_length) {
    return read_string(packet->data, packet->size, &packet->read_pos, target, target_length);
}

