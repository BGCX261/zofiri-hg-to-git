#include "zofiri.h"

#include <string>

using namespace zof;

int main(int argc, char** argv) {
	srand(time(0));
	srand48(time(0));
	Server server(10000);
	// TODO Need some kind of auto.
	Socket* socket = server.accept();
	std::string line;
	socket->readLine(line);
	cout << line.c_str() << endl;
	delete socket;
	Sim sim;
	World world(&sim);
	Viz viz(&sim);
	viz.run();
}
