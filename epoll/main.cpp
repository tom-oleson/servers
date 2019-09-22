/*
 * Copyright (c) 2019, Tom Oleson <tom dot oleson at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * The names of its contributors may NOT be used to endorse or promote
 *     products derived from this software without specific prior written
 *     permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>

#include <errno.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>


#define EXIT_FAILURE 1
#define MAX_EVENTS 10

int port = 54000;              

void enable_reuseaddr(int fd) {
    int enable = 1;
    if (-1 == setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable))) {
        perror("setsockopt: SO_REUSEADDR");
        exit(EXIT_FAILURE);
    }
}

void set_non_blocking(int fd) {

    // get current flags for fd
    int flags = fcntl(fd, F_GETFL);
    if (-1 == flags) {
        perror("fcntl(F_GETFL)");
        exit(EXIT_FAILURE);
    }

    flags |= O_NONBLOCK;

    if (-1 == fcntl(fd, F_SETFL, flags)) {
        perror("fcntl(F_SETFL,O_NONBLOCK)");
        exit(EXIT_FAILURE);
    }
}

int create_listen_socket() {

    // create socket
    //============================
    int host_socket = socket(AF_INET, SOCK_STREAM, 0 /*protocol*/);
    if(-1 == host_socket) {
        perror("not able to create socket!");
        exit(EXIT_FAILURE);
    }

    enable_reuseaddr(host_socket);

    // bind socket to ip and port
    //=============================
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons( port );
    hint.sin_addr.s_addr = htonl(INADDR_ANY);

    if(-1 == bind(host_socket, (sockaddr *) &hint, sizeof(hint))) {
        perror("not able to bind to IP/port!");
        exit(EXIT_FAILURE);
    }

    // mark socket for listening
    //===============================
    if(-1 == listen(host_socket, SOMAXCONN)) {
        perror("not able to listen!");
        exit(EXIT_FAILURE);
    }
    std::cout << "listening on port " << port << "\n";

    return host_socket;
}

int create_epoll_instance() {
    int epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }
    return epollfd;
}

void add_socket(int epollfd, int fd, uint32_t flags) {
    struct epoll_event ev;
    ev.events = flags;
    ev.data.fd = fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }
}

void delete_socket(int epollfd, int fd) {
    struct epoll_event ev;
    ev.events = 0;
    ev.data.fd = fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }
}

int do_accept(int host_socket) { 

    int fd = -1;

    sockaddr_in client_hint;
    socklen_t client_sz = sizeof(client_hint);
    bzero(&client_hint, sizeof(client_hint));

    if(-1 == (fd = ::accept(host_socket, (sockaddr *) &client_hint, &client_sz))) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    char host[NI_MAXHOST] = { '\0' };
    char serv[NI_MAXSERV] = { '\0' };
    char info_buf[NI_MAXHOST + NI_MAXSERV + 1] = { '\0' };

    if( 0 == getnameinfo( (sockaddr *) &client_hint, sizeof(client_hint),
        host, sizeof(host), serv, sizeof(serv), 0 /*flags*/) ) {
        snprintf(info_buf, sizeof(info_buf), "%s:%s", host, serv);
    }
    else if( NULL != inet_ntop(AF_INET, &client_hint, host, sizeof(host) ))  {
        // no name info available, use info from client connection...
        snprintf(info_buf, sizeof(info_buf), "%s:%d", host, ntohs(client_hint.sin_port));
    }

    set_non_blocking(fd);

    std::cout << "connection on: " << info_buf << "\n";

    return fd;
}

void do_use(int epollfd, int fd, char *buf, size_t sz) {

    while(1) {
        
        int num_bytes = read(fd, buf, sz);
        
        if(num_bytes == -1 && errno == EAGAIN) {
            // back to caller for the next epoll_wait()
            return;
        }

        if(num_bytes == 0) {
            // EOF - client disconnected
            // remove socket from interest list...
            delete_socket(epollfd, fd);
            close(fd);

            std::cout << "<" << fd << ">: connection closed.\n";
            return;
        }
        
        if(num_bytes > 0) {
            // got some data, add to the output...
            buf[num_bytes] = '\0';
            std::cout << buf;
        }

        if(num_bytes == -1) {
            perror("read");
            // remove bad connection from interest list...
            delete_socket(epollfd, fd);
            close(fd);
            return;
        }
    }
}

main() {

    /* C/C++ banner */
    const char *__banner__ =
    R"(                  _ _)""\n"
    R"(  ___ _ __   ___ | | |)""\n"
    R"( / _ \ '_ \ / _ \| | |)""\n"
    R"(|  __/ |_) | (_) | | |)""\n"
    R"( \___| .__/ \___/|_|_|)""\n"
    R"(     |_|)""\n"
    "Copyright (C) 2019, Tom Oleson. All Rights Reserved.\n";
    puts(__banner__);


    struct epoll_event ev, events[MAX_EVENTS];
    int conn_sock, nfds, timeout = 60000;
    char buf[1024];

    int listen_sock = create_listen_socket();
    int epollfd = create_epoll_instance();

    // events on this socket means we have a new connection...
    add_socket(epollfd, listen_sock, EPOLLIN);

    while(1) {

        // fetch the ready list (fds that are ready for I/O)
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, timeout);
        if(-1 == nfds) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        // process the ready fds
        for(int n = 0; n < nfds; ++n) {

            int fd = events[n].data.fd;

            if(fd == listen_sock) {
                // we have a new client connection...
                conn_sock = do_accept(listen_sock);
                add_socket(epollfd, conn_sock, EPOLLIN | EPOLLET);
            }
            else {
                // we have new data from a client connection...
                do_use(epollfd, fd, buf, sizeof(buf));
            }
        }
    }
    return 0;
}

