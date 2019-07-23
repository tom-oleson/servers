#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>

using namespace std;


// compile: $ g++ main.cpp -o tcpserver
// test: telnet localhost 54000


int main() {

	const char *__banner__ =
	R"( _)""\n"
	R"(| |_ ___ _ __  ___  ___ _ ____   _____ _ __)""\n"
	R"(| __/ __| '_ \/ __|/ _ \ '__\ \ / / _ \ '__|)""\n"
	R"(| || (__| |_) \__ \  __/ |   \ V /  __/ |)""\n"
	R"( \__\___| .__/|___/\___|_|    \_/ \___|_|)""\n"
	R"(        |_|)""\n"
	"Copyright (C) 2019, Tom Oleson. All Rights Reserved.\n";

	puts(__banner__);


	// create socket
	//============================
	int host_socket = socket(AF_INET, SOCK_STREAM, 0 /*protocol*/);
	if(-1 == host_socket) {
		cerr << "not able to create socket!" << endl;
		return -1;
	}

	// bind socket to ip and port
	//=============================
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(54000);
	hint.sin_addr.s_addr = htonl(INADDR_ANY);

	if(-1 == bind(host_socket, (sockaddr *) &hint, sizeof(hint))) {
		cerr << "not able to bind to IP/port!" << endl;
		return -2;
	}

	// mark socket for listening
	//===============================
	if(-1 == listen(host_socket, SOMAXCONN)) {
		cerr << "not able to listen!" << endl;
		return -3;
	}

	cout << "waiting for connection..." << endl;

	// accept connection
	//===============================
	sockaddr_in client;
	socklen_t client_sz = sizeof(client);
	char host[NI_MAXHOST] = "\0";
	char svc[NI_MAXSERV] = "\0";

	int client_socket = accept(host_socket, (sockaddr *) &client, &client_sz);
	if(-1 == client_socket) {
		cerr << "client socket error!" << endl;
		return -4;
	}

	close(host_socket);


	// log connection to console...
	memset(host, 0, sizeof(host));
	memset(svc, 0, sizeof(svc));

	if( 0 == getnameinfo( (sockaddr *) &client, sizeof(client),
		 host, sizeof(host), svc, sizeof(svc), 0 /*flags*/) ) {
		cout << "<h> connected on " << host << ":" << svc << endl;
	}
	else {
		// no name info available, use info from client connection...
		if(NULL != inet_ntop(AF_INET, &client, host, sizeof(host) )) {
			cout << "<a> connected on " << host <<  ":" <<  ntohs( client.sin_port) << endl;
		}
	} 

	// while message received from client socket; echo message
	char buf[4096] = "\0";

	while(true) {
		memset(buf, 0, sizeof(buf));
		int nob = recv(client_socket, buf, sizeof(buf), 0 /*flags*/);
		if(-1 == nob) {

			cerr << "connection error during recv" << endl;
			break;
		}

		if(0 == nob) {
			cout << "client disconnected" << endl;
			break;
		}

		// local display
		cout << "received[" << nob << "]:" << string(buf, 0, nob) << endl;

		// echo to client
		send(client_socket, buf, nob , 0 /*flags*/);
	}

	close(client_socket);

	return 0;
}

