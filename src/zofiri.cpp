#include "zofiri.h"

#include <iostream>
#include <stdlib.h>
#include <time.h>
#include "pub.h"
#include "sim.h"
#include "viz.h"
#include "world.h"

using namespace zof;

int main(int argc, char** argv) {
	try {
		// Randomize the universe.
		// TODO Better random handling.
		srand(time(0));
		// TODO Note that none of this shows 3rd party dependencies.
		// TODO Hide them for more abstraction and faster builds.
		Sim sim;
		World world(&sim);
		Viz* viz = Viz::create(&sim);
		Pub pub(viz);
		if (argc > 1) {
			pub.mod_uri = argv[1];
		}
		viz->run();
		// TODO auto_ptr
		delete viz;
	} catch (const char* message) {
		cerr << message << endl;
	}
	exit(EXIT_SUCCESS);
}
