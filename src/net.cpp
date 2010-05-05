#include <iostream>
#include "net.h"
//#include <sys/time.h>

using namespace std;

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
	if(needed) {
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
	if(id < 0) {
		throw "failed to open server socket";
	}
	// Allow reuse to avoid restart blocking pain.
	int reuse = 1;
	if(setsockopt(id, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		throw "failed to set reuseaddr on server socket";
	}
	// Get ready to bind and listen.
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
	// Close all open sockets.
	for(vector<Socket*>::iterator socket = sockets.begin(); socket < sockets.end(); socket++) {
		delete *socket;
	}
	// And close the server, too.
	close(id);
	id = 0;
}

Socket* Server::accept() {
	sockaddr_in address;
	size_t addressLength = sizeof(address);
	int socketId = ::accept(id, (sockaddr*)&address, &addressLength);
	if(socketId < 0) {
		throw "failed to accept";
	}
	return new Socket(socketId);
}

void Server::select(vector<Socket*>* sockets) {
	// Clear out any existing entries.
	sockets->clear();
	// First check to see if we have any incoming connections.
	// TODO What should the priority be?
	fd_set ids;
	FD_ZERO(&ids);
	FD_SET(id, &ids);
	for(vector<Socket*>::iterator socket = this->sockets.begin(); socket < this->sockets.end(); socket++) {
		FD_SET((*socket)->id, &ids);
	}
	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	// TODO Would really setting to max socket id + 1 really be better?
	int ready = ::select(FD_SETSIZE, &ids, 0, 0, &timeout);
	if(ready < 0) {
		throw "failed select";
	} else if(ready) {
		if (FD_ISSET(id, &ids)) {
			// New incoming socket. Keep it in our list.
			Socket* socket = accept();
			this->sockets.push_back(socket);
			// Don't actually give it to the user until next time, in case no data is ready.
			// Or should we loop on accepting sockets, then try the open sockets in a next step?
		}
		vector<Socket*>::iterator s = this->sockets.begin();
		while(s < this->sockets.end()) {
			Socket* socket = *s;
			if (FD_ISSET(socket->id, &ids)) {
				if (socket->closed()) {
					s = this->sockets.erase(s);
					delete socket;
					socket = 0;
				} else {
					sockets->push_back(socket);
				}
			}
			if (socket) {
				s++;
			}
		}
	}
}

Socket::Socket(int id) {
	this->id = id;
}

Socket::~Socket() {
	// TODO closesocket for _WIN32
	::close(id);
	id = -1;
}

bool Socket::closed() {
	char c;
	if (id >= 0) {
		if (!recv(id, &c, 1, MSG_PEEK)) {
			::close(id);
			id = -1;
		}
	}
	return id < 0;
}

bool Socket::readLine(std::string* line, bool clear) {
	//long long diff = 0;
	//timeval start, end;
	char c;
	int amount;
	bool any = false;
	if(clear) {
		line->clear();
	}
	//if (closed()) {
	//	return false;
	//}
	while(true) {
		// TODO Change the readline to a generic function.
		// TODO Change the byte giver to one that's buffered?
		//gettimeofday(&start, 0);
		amount = recv(id, &c, 1, 0);
		//gettimeofday(&end, 0);
		//diff += 1000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000;
		if(amount < 0) {
			throw "error reading from socket";
		}
		if(amount) {
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
			*line += c;
		} else {
			// End of stream.
			cerr << "Totally at the end!" << endl;
			::close(id);
			id = -1;
			break;
		}
	}
	// cerr << "clock: " << (0.001 * diff) << endl;
	return any;
}

void Socket::writeLine(const char* text) {
	// cerr << "writeLine: " << text << endl;
	for(; *text; text++) {
		// TODO Is sending in batches better?
		send(id, text, 1, 0);
	}
	send(id, "\n", 1, 0);
}

}
