#include "pub.h"

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
	Socket* socket = server->select();
	if (socket) {
		// Read messages, apply updates, and send data.
	}
}

}
