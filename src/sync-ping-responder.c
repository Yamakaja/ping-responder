#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "protocol.h"

char *response_body;
size_t response_body_size;

void handle_client(struct sockaddr_in *addr, int client) {
    uint8_t buffer[1024];
    size_t read_bytes = read(client, buffer, sizeof(buffer));
    size_t offset = 0;

    mc_packet packet;
    if (init_packet(&packet, buffer, read_bytes, &offset)) {
        printf("Failed to initialize packet! (%s:%d)\n", __FILE__, __LINE__);
        cleanup_packet(&packet);
        return;
    }

    if (packet.id != 0) {
        puts("Received invalid packet ...");
        cleanup_packet(&packet);
        return;
    }

    if (read_packet(&packet, buffer, read_bytes, &offset)) {
        printf("Failed to read packet! (%s:%d)\n", __FILE__, __LINE__);
        cleanup_packet(&packet);
        return;
    }

    int protocol_version;
    char host[128];
    uint16_t port;
    int next_state;

    if (read_packet_var_int(&packet, &protocol_version)
            || read_packet_string(&packet, host, sizeof(host))
            || read_packet_short(&packet, &port)
            || read_packet_var_int(&packet, &next_state)) {
        printf("Failed to read packet! (%s:%d)\n", __FILE__, __LINE__);
        cleanup_packet(&packet);
        return;
    }

    printf("{protocol_version: %d, host: %s, port: %d, nextState: %d}\n", protocol_version, host, port, next_state);
    cleanup_packet(&packet);
}

int main(int argc, char **argv) {
    struct stat file_stat;

    if (stat("response.json", &file_stat)) {
        printf("%s: Failed to load response.json: %s\n", argv[0], strerror(errno));
        return 1;
    }

    response_body_size = file_stat.st_size;
    int response_file = open("response.json", 0);

    if (response_file == -1) {
        printf("%s: Failed to open response.json: %s\n", argv[0], strerror(errno));
        return 1;
    }

    response_body = mmap(NULL, response_body_size, PROT_READ, MAP_PRIVATE, response_file, 0);
    if (response_body == MAP_FAILED) {
        printf("%s: Failed to map response.json into memory: %s\n", argv[0], strerror(errno));
        return 1;
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        printf("%s: Failed to create socket: %s\n", argv[0], strerror(errno));
        return 1;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(25565);

    if (bind(fd, (struct sockaddr *) &server, sizeof(server))) {
        printf("%s: Failed to bind socket: %s\n", argv[0], strerror(errno));
        return 1;
    }

    if (listen(fd, 10)) {
        printf("%s: Failed to listen on socket: %s\n", argv[0], strerror(errno));
        return 1;
    }

    struct sockaddr_in client;
    socklen_t addr_len;
    int client_fd;

    while (1) {
        addr_len = sizeof(client);
        client_fd = accept(fd, (struct sockaddr *) &client, &addr_len); 

        if (client_fd < 0) {
            printf("%s: Failed to accept connection: %s\n", argv[0], strerror(errno));
            continue;
        }

        char *ip = inet_ntoa(client.sin_addr);

        handle_client(&client, client_fd);

        if (close(client_fd)) {
            printf("%s: An error occurred while trying to close connection to %s: %s\n", argv[0], ip, strerror(errno));
            continue;
        }
        
    }

    return 0;
}

