#include <string.h>
#include <unistr.h>

#include "encoding.h"

size_t required_var_int_bytes(int value) {
    if (value < 0)
        return 5;

    if (value < 1 << 7)
        return 1;

    if (value < 1 << 14)
        return 2;

    if (value < 1 << 21)
        return 3;

    if (value < 1 << 28)
        return 4;

    return 5;
}

int read_var_int(uint8_t *buffer, size_t buffer_size, size_t *offset, int32_t *target) {
    int result = 0;
    int round = 0;
    uint8_t cur;

    do {
        if (*offset >= buffer_size || round == 5)
            return -1;

        result |= ((cur = buffer[(*offset)++]) & 0x7F) << 7 * round;
    } while (round++, cur & 0x80);

    *target = result;
    return 0;
}

void write_var_int(uint8_t *buffer, size_t buffer_size, size_t *offset, int32_t value) {
    for (int i = 0; i < 5 && *offset < buffer_size && (i == 0 || (value >> i * 7)); i++) 
        buffer[(*offset)++] = (unsigned char) (((value >> i * 7) & 0x7F) | ((value >> (i + 1) * 7) ? 0x80 : 0x0));
}

void write_string(uint8_t *buffer, size_t buffer_size, size_t *offset, char *str, size_t len) {
    if (u8_strlen((uint8_t *) str) > 32767)
        return;
    
    if (buffer_size - *offset - len - 4 < 0)
        return;
    
    write_var_int(buffer, buffer_size, offset, len);
    memcpy(buffer + *offset, str, len);
    *offset += len;
}

int read_string(uint8_t *buffer, size_t buffer_size, size_t *offset, char *target_buffer, size_t target_buffer_size) {
    int size;
    if (read_var_int(buffer, buffer_size, offset, &size))
        return -1;

    if (size + 1 > target_buffer_size)
        return -1;

    if (size > buffer_size - *offset)
        return -1;

    memcpy(target_buffer, buffer + *offset, size);
    *offset += size;

    target_buffer[size] = 0;
    return 0;
}

void write_short(uint8_t *buffer, size_t buffer_size, size_t *offset, uint16_t value) {
    if (buffer_size - *offset < 2)
        return;

    *((uint16_t*) (buffer + *offset)) = value;
    *offset += 2;
}

int read_short(uint8_t *buffer, size_t buffer_size, size_t *offset, uint16_t *target) {
    if (buffer_size - *offset < 2)
        return -1;

    uint8_t a = *(buffer + (*offset)++);
    uint8_t b = *(buffer + (*offset)++);

    *target = (uint16_t) (0x0U | a << 8 | b);
    return 0;
}



