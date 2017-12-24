#ifndef HEADER_PING_RESPONDER
#define HEADER_PING_RESPONDER

#include "protocol.h"

void handle_event(struct epoll_event *);
void handle_signal(int sig);
void set_non_blocking(int fd);
void setup_signal_handler();
int bind_to(uint16_t port);
int setup_epoll(int listening_socket, struct epoll_event *);

void accept_client();
void handle_client(struct epoll_event *);
int handle_packet(mc_client *client);
void close_client(int fd);

int load_response();

#endif

