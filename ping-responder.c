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
        char buffer[1024];
        size_t read_pos = 0;
        size_t received = recv(client, buffer, sizeof(buffer), 0);

        char *ip = inet_ntoa(addr->sin_addr);
        printf("Accepted connection from: %s\n", ip);

        if (received < 0) {
            printf("An error occurred in connection to %s: %s\n", ip, strerror(errno));
            return;
        }

        if (received < 5)
            return;

        int length = read_var_int(buffer, received, &read_pos);
        if (length > received - read_pos)
            return;

        int packet_id = read_var_int(buffer, received, &read_pos);
        if (packet_id != 0)
            return;

        int protocol_version = read_var_int(buffer, received, &read_pos);

        char host[128];
        if (read_string(buffer, received, &read_pos, host, sizeof(host)))
            return;

        uint16_t port = read_short(buffer, received, &read_pos);
        
        int next_state = read_var_int(buffer, received, &read_pos);

        printf("Protocol: %d, Host: %s, Port: %d, NextState: %d\n", protocol_version, host, port, next_state);

        if (received == read_pos) {
            received = recv(client, buffer, sizeof(buffer), 0);
            read_pos = 0;
        } 

        length = read_var_int(buffer, received, &read_pos);
        packet_id = read_var_int(buffer, received, &read_pos);

        if (packet_id != 0)
            return;

        char response[8192]; 
        size_t write_pos = 0;

        write_var_int(buffer, sizeof(response), &write_pos, (int) (required_var_int_bytes(response_body_size) + response_body_size) + 1);
        write_var_int(buffer, sizeof(response), &write_pos, 0);
        write_string(buffer, sizeof(response), &write_pos, response_body, response_body_size);

        send(client, buffer, write_pos, 0);

        size_t ping_length = recv(client, buffer + write_pos, sizeof(buffer) - write_pos, 0);
        if (ping_length < 0)
            return;
        send(client, buffer + write_pos, ping_length, 0);
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

