#include "pub.h"
#include <string>

namespace zof {

Pub::Pub(Viz* viz, int port) {
	server = new Server(port);
	this->viz = viz;
	viz->pub = this;
}

Pub::~Pub() {
	delete server;
}

void Pub::update() {
	vector<Socket*> sockets;
	server->select(&sockets);
	for(vector<Socket*>::iterator s = sockets.begin(); s < sockets.end(); s++) {
		Socket* socket = *s;
		std::string line;
		socket->readLine(&line);
		cout << "Client says: " << line << endl;
		// TODO Apply updates, and send data.
	}
}

}
