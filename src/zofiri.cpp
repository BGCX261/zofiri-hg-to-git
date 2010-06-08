#include "zofiri.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

using namespace zof;

int main(int argc, char** argv) {
	try {
		if (argc > 1) {
			char* error;
			void* lib = dlopen(argv[1], RTLD_NOW);
			if (!lib) {
				fprintf(stderr, "%s\n", dlerror());
				exit(EXIT_FAILURE);
			}
			void (*init)();
			// Lock something around dlerror calls?
			dlerror();
			// C supposedly likes the former, but g++ likes the latter.
			//*(void**)(&init) = dlsym(lib, "init");
			init = (void(*)())dlsym(lib, "init");
			if ((error = dlerror()) != NULL) {
				fprintf(stderr, "%s\n", error);
			}
			init();
			dlclose(lib);
		}

		// Randomize the universe.
		// TODO Better random handling.
		srand(time(0));
		// TODO Note that none of this shows 3rd party dependencies.
		// TODO Hide them for more abstraction and faster builds.
		Sim sim;
		World world(&sim);
		Viz* viz = Viz::create(&sim);
		Pub pub(viz);
		pub.world = &world;
		viz->run();
		// TODO auto_ptr
		delete viz;
	} catch (const char* message) {
		cout << message << endl;
	}
	exit(0);
}

zof_num zof_num_max(zof_num a, zof_num b) {
	return a > b ? a : b;
}

zof_num zof_num_min(zof_num a, zof_num b) {
	return a < b ? a : b;
}
