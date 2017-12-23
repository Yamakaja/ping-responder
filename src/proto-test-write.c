#include "protocol.h"
#include <stdio.h>

int main() {
    FILE *output = fopen("output.bin", "w");
    
    char buffer[1024];
    size_t write_offset = 0;

    // for (int i = -100; i < 101; i++) {
    //     write_var_int(buffer, sizeof(buffer), &write_offset, i);
    //     if (write_offset > 1000) {
    //         fwrite(buffer, 1, write_offset, output);
    //         write_offset = 0;
    //     }
    // }

    write_string(buffer, sizeof(buffer), &write_offset, "Hello world!", 12);
    fwrite(buffer, 1, write_offset, output);

    fclose(output);
}

