#include "log.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/epoll.h>

#define MAX_EVENTS 16

static int events_num = 0;
static struct epoll_event events[MAX_EVENTS] = {{0}};
static int epoll_fd = 0;

int init_event() {
    if ((epoll_fd = epoll_create1(0)) == -1) {
        log_perror("epoll_create1");
        return -1;
    }
    return 0;
}

int add_event(int fd, int flag) {
    struct epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = flag;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        log_perror("epoll_ctl");
        return -1;
    }
    return 0;
}

int modify_event(int fd, int flag) {
    struct epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = flag;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev) == -1) {
        log_perror("epoll_ctl");
        return -1;
    }
    return 0;
}

struct epoll_event *wait_event() {
    if (events_num <= 0) {
        events_num = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    }
    if (events_num == -1) { //error
        if (errno != EINTR) {
            log_perror("epoll");
        }
        return NULL;
    }

    if (events_num == 0) { //not found
        return NULL;
    }
    events_num--;
    return &events[events_num];
}
