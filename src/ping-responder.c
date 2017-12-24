#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/stat.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>

#include "ping-responder.h"
#include "dict.h"

#define MAX_EVENTS 64

int listening_socket;
int epoll_instance;

uint8_t *response_body;
size_t response_body_size;

dict clients;

int main(int argc, char **argv) {
    setup_signal_handler();

    if (load_response())
        return EXIT_FAILURE;

    listening_socket = bind_to(25565);
    if (listening_socket == -1)
        return EXIT_FAILURE;

    struct epoll_event ev, events[MAX_EVENTS];

    epoll_instance = setup_epoll(listening_socket, &ev);
    if (epoll_instance < 0)
        return EXIT_FAILURE;

    clients = dict_init(8);

    for (;;) {
        int nfds = epoll_wait(epoll_instance, events, MAX_EVENTS, -1);

        if (nfds == -1) {
            printf("epoll_wait failed: %s\n", strerror(errno));
            return EXIT_FAILURE;
        }

        for (int i = 0; i < nfds; i++)
            handle_event(&events[i]);
    }
}


void handle_event(struct epoll_event *event) {
    if (event->data.fd == listening_socket)
        accept_client();
    else
        handle_client(event);

}

void handle_client(struct epoll_event *event) {
    if (event->events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
        close_client(event->data.fd);
        return;
    }

    uint8_t buffer[2048];
    size_t offset = 0;
    ssize_t len;

    mc_client *client = *dict_get(&clients, event->data.fd);

    for (;;) {
        len = read(event->data.fd, buffer, sizeof(buffer));
        offset = 0;

        if (len == 0) {
            close_client(client->fd);
            break;
        }

        if (len < 0) {
            if (errno == EAGAIN)
                break;

            printf("An error occurred while handling connection: %s\n", strerror(errno));
            close_client(client->fd);
            break;
        }

        while (len - offset > 0) {
            printf("len: %ld, offset: %ld\n", len, offset);
            if (client->packet == NULL) {
                client->packet = calloc(1, sizeof(mc_packet));
                init_packet(client->packet, buffer, len, &offset);
            }

            read_packet(client->packet, buffer, len, &offset);
            printf("len: %ld, offset: %ld\n", len, offset);

            if (client->packet->status & PKT_COMPLETE) {
                if (handle_packet(client)) {
                    close_client(client->fd);
                    return;
                }

                cleanup_packet(client->packet);
                free(client->packet);
                client->packet = NULL;
                continue;
            } else if(client->packet->status & PKT_INCOMPLETE) {
                break;
            } else {
                close_client(client->fd);
                return;
            }
        }
    }
}

int handle_packet(mc_client *client) {
    switch (client->state) {
        case STATE_PRE_HANDSHAKE:
            {
                if (client->packet->id != 0) {
                    puts("Received packet with non-zero id during handshake!");
                    return -1;
                }

                int protocol_version;
                char host[128];
                uint16_t port;
                int next_state;

                if (read_packet_var_int(client->packet, &protocol_version)
                        || read_packet_string(client->packet, host, sizeof(host))
                        || read_packet_short(client->packet, &port)
                        || read_packet_var_int(client->packet, &next_state)) {
                    printf("Failed to read packet! (%s:%d)\n", __FILE__, __LINE__);
                    return -1;
                }

                printf("{protocol_version: %d, host: %s, port: %d, nextState: %d}\n", protocol_version, host, port, next_state);

                if (next_state != 1)
                    return -1;

                client->state = STATE_PRE_REQUEST;
                return 0;
            }
        case STATE_PRE_REQUEST:
            {
                if (client->packet->id != 0) {
                    puts("Received packet with non-zero id during status request!");
                    return -1;
                }

                write(client->fd, response_body, response_body_size);

                client->state = STATE_PRE_PING;
                return 0;
            }
        case STATE_PRE_PING:
            {
                if (client->packet->id != 1) {
                    puts("Received packet with non-one id during ping request!");
                    return -1;
                }

                if (client->packet->size > 20) {
                    puts("Received too large ping!");
                    return -1;
                }

                uint8_t buffer[32];
                size_t offset = 0;
                write_var_int(buffer, sizeof(buffer), &offset, client->packet->size + 1);
                write_var_int(buffer, sizeof(buffer), &offset, 1);
                memcpy(buffer + offset, client->packet->data, client->packet->size);
                write(client->fd, buffer, offset + client->packet->size);
                return -1;
            }
    }
    return -1;
}

void close_client(int fd) {
    mc_client **client = (mc_client **) dict_get(&clients, fd);

    if (*client != NULL) {
        if ((*client)->packet != NULL)
            cleanup_packet((*client)->packet);

        free((*client)->packet);
    }

    free(*client);
    *client = NULL;

    close(fd);
}

void accept_client() {
    mc_client *client = calloc(1, sizeof(mc_client));
    client->packet = NULL;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    client->fd = accept(listening_socket, (struct sockaddr *) &client->addr, &addr_len);
    if (client->fd == -1) {
        printf("An error occurred while trying to accept connection: %s\n", strerror(errno));
        free(client);
        return;
    }

    set_non_blocking(client->fd);

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = client->fd;

    if (epoll_ctl(epoll_instance, EPOLL_CTL_ADD, client->fd, &ev) == -1) {
        printf("An error occurred while trying to register new client with epoll instance: %s\n", strerror(errno));
        close(client->fd);
        free(client);
        return;
    }

    client->state = STATE_PRE_HANDSHAKE;
    *dict_get(&clients, client->fd) = client;
}

int setup_epoll(int l_socket, struct epoll_event *ev) {
    int epoll_sock = epoll_create1(0);
    if (epoll_sock == -1) {
        printf("Failed to create epoll instance: %s\n", strerror(errno));
        return -1;
    }

    ev->events = EPOLLIN;
    ev->data.fd = listening_socket;
    if (epoll_ctl(epoll_sock, EPOLL_CTL_ADD, l_socket, ev) == -1) {
        printf("Failed to add listening socket to epoll instance: %s\n", strerror(errno));
        return -1;
    }

    return epoll_sock;
}

int bind_to(uint16_t port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("Failed to open socket: %s\n", strerror(errno));
        return -1;
    }

    int val = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int))) {
        printf("Failed to set socket option: %s\n", strerror(errno));
        return -1;
    }

    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr))) {
        printf("Failed to bind socket: %s\n", strerror(errno));
        return -1;
    }

    if (listen(sock, 10)) {
        printf("Failed to listen on socket: %s\n", strerror(errno));
        return -1;
    }

    return sock;
}

void set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL);

    if (flags == -1) {
        printf("Couldn't get socket options: %s\n", strerror(errno));
        return;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        printf("Couldn't set socket option: %s\n", strerror(errno));
        return;
    }
}

void setup_signal_handler() {
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sa.sa_flags = SA_RESTART;
    sigfillset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1)
        printf("Failed to attach SIGINT signal handler: %s\n", strerror(errno));
    
    if (sigaction(SIGHUP, &sa, NULL) == -1)
        printf("Failed to attach SIGHUP signal handler: %s\n", strerror(errno));
}

void handle_signal(int sig) {
    for (int i = 0; i < clients.capacity; i++)
        if (clients.data[i] != NULL)
            close_client(i);

    dict_cleanup(&clients);

    close(listening_socket);
    close(epoll_instance);

    free(response_body);

    exit(EXIT_SUCCESS);
}

int load_response() {
    struct stat file_stat;

    if (stat("response.json", &file_stat)) {
        printf("Failed to stat response.json: %s\n", strerror(errno));
        return -1;
    }

    response_body_size = required_var_int_bytes(file_stat.st_size)
        + required_var_int_bytes(file_stat.st_size + required_var_int_bytes(file_stat.st_size) + 1)
        + file_stat.st_size
        + 1;

    response_body = calloc(1, response_body_size);

    int fd = open("response.json", 0);
    if (fd == -1) {
        printf("Failed to open response.json: %s\n", strerror(errno));
        return -1;
    }

    size_t write_offset = 0;

    write_var_int(response_body, response_body_size, &write_offset, 1 + required_var_int_bytes(file_stat.st_size) + file_stat.st_size);
    write_offset++;
    write_var_int(response_body, response_body_size, &write_offset, file_stat.st_size);

    read(fd, response_body + write_offset, file_stat.st_size);
    close(fd);

    return 0;
}

