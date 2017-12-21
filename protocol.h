#ifndef HEADER_PROTOCOL
#define HEADER_PROTOCOL

#include <unistd.h>
#include <stdint.h>

int read_var_int(char *buffer, size_t buffer_size, size_t *offset);
void write_var_int(char *buffer, size_t buffer_size, size_t *offset, int value);
size_t required_var_int_bytes(int value);
void write_string(char *buffer, size_t buffer_size, size_t *offset, char *str, size_t len);
int read_string(char *buffer, size_t buffer_size, size_t *offset, char *target_buffer, size_t target_buffer_size);
uint16_t read_short(char *buffer, size_t buffer_size, size_t *offset);
void write_short(char *buffer, size_t buffer_size, size_t *offset, uint16_t value);

#endif

