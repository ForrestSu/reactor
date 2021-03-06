/*
 * reactor.cc
 */

#include "reactor.hh"
#include <cassert>
#include <unistd.h>
#include <fcntl.h>

reactor::reactor()
    : _epollfd(epoll_create1(EPOLL_CLOEXEC)) {
    assert(_epollfd != -1);
}

reactor::~reactor() {
    ::close(_epollfd);
}

void reactor::epoll_add_in(pollable_fd& pfd, std::unique_ptr<task> t) {
    auto ctl = pfd.events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    pfd.events |= EPOLLIN;
    assert(!pfd.pollin);
    pfd.pollin = std::move(t);
    ::epoll_event eevt;
    eevt.events = pfd.events;
    eevt.data.ptr = &pfd;
    int r = ::epoll_ctl(_epollfd, ctl, pfd.fd, &eevt);
    assert(r == 0);
}

void reactor::epoll_add_out(pollable_fd& pfd, std::unique_ptr<task> t) {
    auto ctl = pfd.events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    pfd.events |= EPOLLOUT;
    assert(!pfd.pollout);
    pfd.pollout = std::move(t);
    ::epoll_event eevt;
    eevt.events = pfd.events;
    eevt.data.ptr = &pfd;
    int r = ::epoll_ctl(_epollfd, ctl, pfd.fd, &eevt);
    assert(r == 0);
}

std::unique_ptr<pollable_fd>
reactor::listen(socket_address sa)
{
    int fd = ::socket(sa.u.sa.sa_family, SOCK_STREAM, SOCK_NONBLOCK | SOCK_CLOEXEC);
    assert(fd != -1);
    int r = ::bind(fd, &sa.u.sa, sizeof(sa.u.sas));
    assert(r != -1);
    ::listen(fd, 100);
    return std::unique_ptr<pollable_fd>(new pollable_fd(fd));
}

void reactor::run() {
    std::array<epoll_event, 128> eevt;
    while (true) {
        int nr = ::epoll_wait(_epollfd, eevt.data(), eevt.size(), -1);
        if (nr == -1) {
            // EINTR buy strace
            if(errno == EINTR) {
                printf("epoll_wait() interrupted 4!\n");
                continue;
            }
            printf("epoll_wait() errno %d<%s>!\n", errno, strerror(errno));
            break;
        }
        for (int i = 0; i < nr; ++i) {
            auto& evt = eevt[i];
            auto pfd = static_cast<pollable_fd*>(evt.data.ptr);
            auto events = evt.events;
            if (events & EPOLLIN) {
                auto t = std::move(pfd->pollin);
                t->run();
            }
            if (events & EPOLLOUT) {
                auto t = std::move(pfd->pollout);
                t->run();
            }
        }
    }
}

socket_address make_ipv4_address(ipv4_addr addr) {
    socket_address sa;
    sa.u.in.sin_family = AF_INET;
    sa.u.in.sin_port = htons(addr.port);
    std::memcpy(&sa.u.in.sin_addr, addr.host, 4);
    return sa;
}

