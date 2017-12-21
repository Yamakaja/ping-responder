#include <string.h>
#include <unistr.h>

#include "protocol.h"

size_t required_var_int_bytes(int value) {
    if (value < 0)
        return 5;

    if (value < 1 << 6)
        return 1;

    if (value < 1 << 13)
        return 2;

    if (value < 1 << 21)
        return 3;

    if (value < 1 << 28)
        return 4;

    return 5;
}

int read_var_int(char *buffer, size_t buffer_size, size_t *offset) {
    int result = 0;
    char cur = 0x80;
    for (int i = 0; i < 5 && *offset < buffer_size && (cur & 0x80); i++)
        result |= ((cur = buffer[(*offset)++]) & 0x7F) << 7 * i;

    return result;
}

void write_var_int(char *buffer, size_t buffer_size, size_t *offset, int value) {
    for (int i = 0; i < 5 && *offset < buffer_size && (i == 0 || (value >> i * 7)); i++) 
        buffer[(*offset)++] = (unsigned char) (((value >> i * 7) & 0x7F) | ((value >> (i + 1) * 7) ? 0x80 : 0x0));
}

void write_string(char *buffer, size_t buffer_size, size_t *offset, char *str, size_t len) {
    if (u8_strlen((uint8_t *) str) > 32767)
        return;
    
    if (buffer_size - *offset - len - 4 < 0)
        return;
    
    write_var_int(buffer, buffer_size, offset, len);
    memcpy(buffer + *offset, str, len);
    *offset += len;
}

int read_string(char *buffer, size_t buffer_size, size_t *offset, char *target_buffer, size_t target_buffer_size) {
    int size = read_var_int(buffer, buffer_size, offset);

    if (size + 1 > target_buffer_size)
        return -1;

    if (size > buffer_size - *offset)
        return -1;

    memcpy(target_buffer, buffer + *offset, size);
    *offset += size;

    target_buffer[size] = 0;
    return 0;
}

void write_short(char *buffer, size_t buffer_size, size_t *offset, uint16_t value) {
    if (buffer_size - *offset < 2)
        return;

    *((uint16_t*) (buffer + *offset)) = value;
    *offset += 2;
}

uint16_t read_short(char *buffer, size_t buffer_size, size_t *offset) {
    if (buffer_size - *offset < 2)
        return 0;

    uint8_t a = *(buffer + (*offset)++);
    uint8_t b = *(buffer + (*offset)++);

    return (uint16_t) (0x0U | a << 8 | b);
}



