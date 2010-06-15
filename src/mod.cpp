#include <dlfcn.h>
#include <iostream>
#include <stdlib.h>
#include <zofiri.h>

namespace zof {

struct Mod: Any {

	virtual ~Mod() {
		dlclose(lib);
	}

	void* lib;
	zof_str uri;

};

}

using namespace zof;

extern "C" {

zof_mod zof_mod_new(zof_str uri) {
	void* lib = dlopen(uri, RTLD_NOW);
	if (!lib) {
		// TODO Store error instead of printing.
		cerr << dlerror() << endl;
		return NULL;
	}
	Mod* mod = new Mod;
	mod->lib = lib;
	// TODO Copy the uri first?
	mod->uri = uri;
	return (zof_mod)mod;
}

zof_str zof_mod_uri(zof_mod mod) {
	return ((Mod*)mod)->uri;
}

zof_bool zof_mod_sim_init(zof_mod mod, zof_sim sim) {
	Mod* mod_struct = (Mod*)mod;
	zof_bool (*sim_init)(zof_mod,zof_sim);
	// Lock something around dlerror calls?
	dlerror();
	// C supposedly likes the former, but g++ likes the latter.
	//*(void**)(&sim_init) = dlsym(lib, "sim_init");
	sim_init = (zof_bool(*)(zof_mod,zof_sim))dlsym(mod_struct->lib, "sim_init");
	char* error;
	if ((error = dlerror()) != NULL) {
		cerr << error << endl;
		return zof_false;
	}
	return sim_init(mod, sim);
}

}
