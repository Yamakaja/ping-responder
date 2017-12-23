#include "protocol.h"
#include <stdio.h>
#include <string.h>

int main() {
    FILE *input = fopen("output.bin", "r");
    
    char buffer[1024];
    size_t read_offset = 0;

    char str[512];
    size_t read_bytes = fread(buffer, 1, sizeof(buffer), input);
    read_string(buffer, read_bytes, &read_offset, str, sizeof(str));
    
    puts(str);
    fclose(input);
}

