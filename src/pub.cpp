#include "pub.h"
#include <string>

namespace zof {

struct AddBodyCommand: public Command {
	virtual std::string perform(Transaction* tx) {
		Sim* sim = tx->pub->viz->sim;
		btRigidBody* body = sim->createBody(
			new btBoxShape(sim->m(btVector3(0.1,0.1,0.1))),
			btTransform(btQuaternion::getIdentity(), sim->m(btVector3(0.0,2.0,0.0)))
		);
		sim->addBody(body);
		return "";
	}
};

void initCommands(Pub* pub) {
	pub->commands["addBody"] = new AddBodyCommand();
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

void Pub::update() {
	vector<Socket*> sockets;
	server->select(&sockets);
	for(vector<Socket*>::iterator s = sockets.begin(); s < sockets.end(); s++) {
		Socket* socket = *s;
		std::string line;
		// TODO We need this non-blocking and caching between sessions.
		// TODO For now we could get hung still.
		// TODO Create transaction object!!!
		Transaction tx(this);
		while (socket->readLine(&line) && line != ";") {
			tx.processLine(line);
		}
		// TODO Apply updates, and send data.
	}
}

Transaction::Transaction(Pub* pub) {
	this->pub = pub;
}

std::string Transaction::processCommand(const vector<std::string>& args) {
	std::string result;
	this->args = args;
	if (args.empty()) {
		return result;
	}
	std::string commandName = args.front();
	std::string varName;
	if (commandName[0] == '$') {
		varName = commandName;
		if (args.size() < 2 || args[1] != "=") {
			throw "var assignment without =";
		}
		this->args.erase(this->args.begin(), this->args.begin() + 2);
		if (this->args.empty()) {
			return result;
		}
		commandName = this->args.front();
	}
	// TODO Perform var substitution among the args.
	std::map<std::string,Command*>::iterator c = pub->commands.find(commandName);
	if (c != pub->commands.end()) {
		result = c->second->perform(this);
	}
	if (!varName.empty()) {
		vars[varName] = result;
	}
	return result;
}

std::string Transaction::processLine(const std::string& line) {
	const char* delims = " \t";
	bool last = false;
	// TODO Handle quotes. Double and single per bash?
	std::string::size_type pos = line.find_first_not_of(delims);
	if (pos == std::string::npos) {
		// Whitespace only.
		return "";
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
	return processCommand(args);
}

}
