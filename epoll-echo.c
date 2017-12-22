#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_EVENTS 64

void handle_event(struct epoll_event *event);
void handle_signal(int sig);

int sock, epoll_sock;

void setnonblocking(int fd) {
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

void handle_signal(int sig) {
    close(sock);
    close(epoll_sock);

    exit(0);
}

int main(int arc, char **argv) {
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sa.sa_flags = SA_RESTART;
    sigfillset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1)
        printf("%s: Failed to intercept SIGINT: %s\n", argv[0], strerror(errno));

    if (sigaction(SIGHUP, &sa, NULL) == -1)
        printf("%s: Failed to intercept SIGHUP: %s\n", argv[0], strerror(errno));

    sock = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sock < 0) {
        printf("%s: An error occurred while trying to open socket: %s\n", argv[0], strerror(errno));
        return 1;
    }

    int val = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int))) {
            printf("%s: Failed to set socket option: %s\n", argv[0], strerror(errno));
            return 1;
    }

    struct sockaddr_in addr;
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12321);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr))) {
        printf("%s: Failed to bind socket: %s\n", argv[0], strerror(errno));
        return 1;
    }

    if (listen(sock, 10)) {
        printf("%s: Failed to listen on socket: %s\n", argv[0], strerror(errno));
        return 1;
    }

    struct epoll_event ev, events[MAX_EVENTS];

    epoll_sock = epoll_create1(0);
    if (epoll_sock == -1) {
        printf("%s: Failed to create epoll instance: %s\n", argv[0], strerror(errno));
        return 1;
    }

    ev.events = EPOLLIN;
    ev.data.fd = sock;
    if (epoll_ctl(epoll_sock, EPOLL_CTL_ADD, sock, &ev) == -1) {
        printf("%s: An error occurred whily trying to add listening socket to epoll: %s\n", argv[0], strerror(errno));
        return 1;
    }

    struct sockaddr_in client;
    socklen_t addr_len;
    int client_fd;

    while (1) {
        int nfds = epoll_wait(epoll_sock, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            printf("%s: An error occurred while trying to wait for epoll events: %s\n", argv[0], strerror(errno));
            return 1;
        }

        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == sock) {
                addr_len = sizeof(struct sockaddr_in);
                client_fd = accept(sock, (struct sockaddr *) &client, &addr_len);

                if (client_fd == -1) {
                    printf("%s: An error occurred while trying to accept connection: %s\n", argv[0], strerror(errno));
                    continue;
                }

                setnonblocking(client_fd);

                ev.events = EPOLLIN;
                ev.data.fd = client_fd;
                if (epoll_ctl(epoll_sock, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                    printf("%s: An error occurred while trying to register client with epoll instance: %s\n", argv[0], strerror(errno));
                    close(client_fd);
                    continue;
                }
            } else {
                handle_event(&events[n]);
            }
        }
    }
}

void handle_event(struct epoll_event *e) {
    if (e->events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
            close(e->data.fd);
            return;
    }

    if (!(e->events & EPOLLIN))
        return;

    char buffer[1024];
    ssize_t len;

    for(;;) {
        len = read(e->data.fd, buffer, sizeof(buffer));
        if (len == 0) {
            close(e->data.fd);
            return;
        }

        if (len < 0) {
            if (errno == EAGAIN)
                break;

            printf("An error occured while handling connection: %s\n", strerror(errno));
            close(e->data.fd);
            break;
        }

        write(e->data.fd, buffer, len);
    }
}

