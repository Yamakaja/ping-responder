#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

void handle_signal(int);

volatile int received_signal = 0;

int main(int argc, char **argv) {
    struct sigaction sa;
    sa.sa_handler = &handle_signal;
    sa.sa_flags = SA_RESTART;
    sigfillset(&sa.sa_mask);

    if (sigaction(SIGHUP, &sa, NULL) == -1)
        printf("%s: Can't intercept SIGHUP: %s\n", strerror(errno));

    if (sigaction(SIGINT, &sa, NULL) == -1)
        printf("%s: Can't intercept SIGINT: %s\n", strerror(errno));

    if (sigaction(SIGUSR1, &sa, NULL) == -1)
        printf("%s: Can't intercept SIGUSR1: %s\n", strerror(errno));

    if (sigaction(SIGUSR2, &sa, NULL) == -1)
        printf("%s: Can't intercept SIGUSR2: %s\n", strerror(errno));

    int time = 60;

    while (time > 0) {
        time = sleep(time);

        switch (received_signal) {
            case 0:
                puts("NONE");
                break;
            case SIGINT:
                puts("SIGINT");
                break;
            case SIGHUP:
                puts("SIGHUP");
                break;
            case SIGUSR1:
                puts("SIGUSR1");
                break;
            case SIGUSR2:
                puts("SIGUSR2");
                break;
        }
    }
}

void handle_signal(int sig) {
    received_signal = sig;
}


