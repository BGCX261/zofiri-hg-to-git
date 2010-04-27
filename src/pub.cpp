#include "pub.h"
#include <string>

namespace zof {

struct AddBodyCommand: public Command {
	AddBodyCommand(Pub* pub): pub(pub) {}
	virtual std::string perform(const vector<std::string>& args) {
		Sim* sim = pub->viz->sim;
		btRigidBody* body = sim->createBody(
			new btBoxShape(sim->m(btVector3(0.1,0.1,0.1))),
			btTransform(btQuaternion::getIdentity(), pub->viz->sim->m(btVector3(0.0,2.0,0.0)))
		);
		sim->addBody(body);
		return "";
	}
	Pub* pub;
};

void initCommands(Pub* pub) {
	pub->commands["addBody"] = new AddBodyCommand(pub);
}

Pub::Pub(Viz* viz, int port) {
	server = new Server(port);
	this->viz = viz;
	viz->pub = this;
	initCommands(this);
}

Pub::~Pub() {
	delete server;
	for (std::map<std::string,Command*>::iterator c = commands.begin(); c != commands.end(); c++) {
		delete c->second;
	}
}

void Pub::processCommand(const vector<std::string>& args) {
	std::string commandName = args.front();
	// TODO Use map of commands to callbacks.
	std::map<std::string,Command*>::iterator c = commands.find(commandName);
	if (c != commands.end()) {
		c->second->perform(args);
	} else if (commandName == "reset") {
		cout << "Yeah!! -> " << commandName << endl;
		world->reset();
	}
}

void Pub::processLine(const std::string& line) {
	const char* delims = " \t";
	bool last = false;
	// TODO Track indent level?
	std::string::size_type pos = line.find_first_not_of(delims);
	if (pos == std::string::npos) {
		// Whitespace only.
		return;
	}
	vector<std::string> args;
	while (!last) {
		std::string::size_type nextPos = line.find_first_of(delims, pos);
		if (nextPos == std::string::npos) {
			nextPos = line.length();
			last = true;
		}
		std::string part = line.substr(pos, nextPos - pos);
		args.push_back(part);
		if (!last) {
			pos = line.find_first_not_of(delims, nextPos);
			if (pos == std::string::npos) {
				last = true;
			}
		}
	}
	processCommand(args);
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
