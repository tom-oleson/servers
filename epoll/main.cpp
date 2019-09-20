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

void do_use(int epollfd, int fd) {

	char buf[1024];

	while(1) {
		// read input
		int num_bytes = read(fd, buf, sizeof(buf));
		if(num_bytes == -1 && errno == EAGAIN) {
			return;
		}
		if(num_bytes > 0) {
			buf[num_bytes] = '\0';
			std::cout << buf;
		}
		else if(num_bytes == 0) {
			
			// EOF
			// remove socket from interest list...
			delete_socket(epollfd, fd);
			close(fd);
			std::cout << "<" << fd << ">: connection closed.\n";
			return;

		} else if(num_bytes == -1) {
			perror("do_read");
			exit(EXIT_FAILURE);
		}
	}
}

main() {

    struct epoll_event ev, events[MAX_EVENTS];
    int conn_sock, nfds, timeout = 60000;

    int listen_sock = create_listen_socket();
    int epollfd = create_epoll_instance();

    add_socket(epollfd, listen_sock, EPOLLIN);

    while(1) {

        // fetch fds that are ready for I/O...
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, timeout);
        if(-1 == nfds) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        // process the ready fds
        for(int n = 0; n < nfds; ++n) {

          int fd = events[n].data.fd;

          if(fd == listen_sock) {
           conn_sock = do_accept(listen_sock);
           add_socket(epollfd, conn_sock, EPOLLIN | EPOLLET);
       }
       else {

           do_use(epollfd, fd);
       }
   }
}

    return 0;
}

