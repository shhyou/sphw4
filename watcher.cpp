#include <cstdio>
#include <cerrno>

#include <unordered_map>

#include <unistd.h>
#include <sys/epoll.h>

#include "watcher.h"

using std::unordered_map;

Listener::~Listener() {}

Watcher::Watcher() {
    epollfd = epoll_create(514);
    if (epollfd < 0) {
        perror("epoll_create");
        throw "epoll_create";
    }
}

Watcher::~Watcher() {}

Watcher& Watcher::getInstance() {
    static Watcher inst;
    return inst;
}

void Watcher::watch(int fd, Listener *receiver){
    epoll_event e;
    e.events = EPOLLIN | EPOLLOUT;
    e.data.fd = fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &e) < 0) {
        perror("epoll_ctl");
        throw "epoll_ctl";
    }
    watching[fd] = receiver;
}

void Watcher::unwatch(int fd) {
    epoll_event e;
    unordered_map<int, Listener*>::iterator it;

    e.events = EPOLLIN | EPOLLOUT;
    e.data.fd = fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &e) < 0) {
        perror("epoll_ctl");
    }

    it = watching.find(fd);
    if (it != watching.end())
        watching.erase(it);
}

void Watcher::wait() {
    epoll_event evs[64];
    unordered_map<int, Listener*>::iterator it;
    int i, e, nfds = epoll_wait(epollfd, evs, sizeof(evs)/sizeof(epoll_event), -1);

    if (nfds < 0) {
        if (errno == EINTR)
            return;
        perror("epoll_wait");
        throw "epoll_wait";
    }

    for (i = 0; i < nfds; i++) {
        it = watching.find(evs[i].data.fd);
        if (it == watching.end())
            continue;
        e = 0;
        if (evs[i].events & EPOLLIN) e |= E_READ;
        if (evs[i].events & EPOLLOUT) e |= E_WRITE;
        if (evs[i].events & EPOLLHUP) e |= E_HANGUP;
        if (evs[i].events & EPOLLERR) e |= E_ERROR;
        it->second->action(it->second, e);
    }
}

