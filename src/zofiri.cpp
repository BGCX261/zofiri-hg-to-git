#include "zofiri.h"

// TODO Nix dlfcn.h here!!
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
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
			zof_mod mod = zof_mod_new(argv[1]);
			if (mod) {
				zof_mod_sim_init(mod, NULL);
				zof_ref_free(mod);
			}
		}

		viz->run();
		// TODO auto_ptr
		delete viz;
	} catch (const char* message) {
		cout << message << endl;
	}
	exit(0);
}

extern "C" {

zof_num zof_num_max(zof_num a, zof_num b) {
	return a > b ? a : b;
}

zof_num zof_num_min(zof_num a, zof_num b) {
	return a < b ? a : b;
}

void zof_ref_free(zof_any ref) {
	zof_type* type = *(zof_type**)ref;
	type->close(ref);
	free(ref);
}

}
