#include "net.h"

#ifdef _WIN32
	// Include winsock stuff.
#else
	#include <stdio.h>
	#include <string.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
#endif

namespace zof {

void initSockets() {
	static bool needed = true;
	if (needed) {
		#ifdef _WIN32
			// Winsock initialization.
		#endif
		needed = false;
	}
}

Server::Server(int port) {
	initSockets();
	// TODO Anything else change here for Windows?
	id = socket(AF_INET, SOCK_STREAM, 0);
	if (id < 0) {
		throw "failed to open server socket";
	}
	sockaddr_in serverAddress;
	bzero(&serverAddress, sizeof(serverAddress));
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(port);
	if (bind(id, (sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
		throw "failed to bind server socket";
	}
	if (listen(id, SOMAXCONN) < 0) {
		throw "failed to listen";
	}
}

Server::~Server() {
	// TODO closesocket for _WIN32
	close(id);
}

Socket* Server::accept() {
	sockaddr_in address;
	size_t addressLength = sizeof(address);
	int socketId = ::accept(id, (sockaddr*)&address, &addressLength);
	if (socketId < 0) {
		throw "failed to accept";
	}
	return new Socket(socketId);
}

Socket::Socket(int id) {
	this->id = id;
}

Socket::~Socket() {
	// TODO closesocket for _WIN32
	::close(id);
	id = 0;
}

bool Socket::readLine(std::string& line, bool clear) {
	char c;
	int amount;
	bool any = false;
	if (clear) {
		line.clear();
	}
	while (true) {
		// TODO Change the readline to a generic function.
		// TODO Change the byte giver to one that's buffered?
		amount = recv(id, &c, 1, 0);
		if (amount < 0) {
			throw "error reading from socket";
		}
		if (amount) {
			any = true;
			if (c == '\r' || c == '\n') {
				if (c == '\r') {
					amount = recv(id, &c, 1, MSG_PEEK);
					if (amount > 0 && c == '\n') {
						// It's a newline as expected. Consume it.
						continue;
					}
				}
				break;
			}
			// TODO Probably more efficient to push chunks.
			line += c;
		} else {
			// End of stream.
			break;
		}
	}
	return any;
}

void Socket::writeLine(char* text) {
	for (; *text; text++) {
		// TODO Is sending in batches better?
		send(id, text, 1, 0);
	}
	send(id, "\n", 1, 0);
}

}
