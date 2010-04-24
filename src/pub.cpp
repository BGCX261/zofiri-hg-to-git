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

void Pub::processLine(const std::string& line) {
	const char* delims = " \t";
	bool last = false;
	size_t pos = 0;
	while (!last) {
		size_t nextPos = line.find_first_of(delims, pos);
		if (nextPos == std::string::npos) {
			nextPos = line.length();
			last = true;
		}
		std::string part = line.substr(pos, nextPos - pos);
		cout << part << endl;
		if (!last) {
			pos = line.find_first_not_of(delims, nextPos);
			if (pos == std::string::npos) {
				last = true;
			}
		}
	}
}

void Pub::update() {
	vector<Socket*> sockets;
	server->select(&sockets);
	for(vector<Socket*>::iterator s = sockets.begin(); s < sockets.end(); s++) {
		Socket* socket = *s;
		std::string line;
		// TODO We need this non-blocking and caching between sessions.
		// TODO For now we could get hung still.
		while (socket->readLine(&line) && line != ";") {
			processLine(line);
		}
		// TODO Apply updates, and send data.
	}
}

}
