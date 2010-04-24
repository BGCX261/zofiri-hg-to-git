#include "zofiri.h"

#include <stdlib.h>
#include <string>

using namespace zof;

int main(int argc, char** argv) {
	try {
		// Randomize the universe.
		srand(time(0));
		srand48(time(0));
		// TODO Note that none of this shows 3rd party dependencies.
		// TODO Hide them for more abstraction and faster builds.
		Sim sim;
		World world(&sim);
		Viz viz(&sim);
		Pub pub(&viz);
		pub.world = &world;
		viz.run();
	} catch (const char* message) {
		cout << message << endl;
	}
	exit(0);
}
