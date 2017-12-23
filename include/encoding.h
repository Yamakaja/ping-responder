#ifndef HEADER_ENCODING
#define HEADER_ENCODING

#include <unistd.h>
#include <stdint.h>

int read_var_int(uint8_t *buffer, size_t buffer_size, size_t *offset, int *target);
void write_var_int(uint8_t *buffer, size_t buffer_size, size_t *offset, int value);
size_t required_var_int_bytes(int value);

int read_string(uint8_t *buffer, size_t buffer_size, size_t *offset, char *target_buffer, size_t target_buffer_size);
void write_string(uint8_t *buffer, size_t buffer_size, size_t *offset, char *str, size_t len);

int read_short(uint8_t *buffer, size_t buffer_size, size_t *offset, uint16_t *target);
void write_short(uint8_t *buffer, size_t buffer_size, size_t *offset, uint16_t value);

#endif

