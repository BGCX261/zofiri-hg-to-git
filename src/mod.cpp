#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <zof.h>

namespace zof {

struct Mod {
	zof_type type;
	void* lib;
	zof_str uri;
};

}

using namespace zof;

extern "C" {

void zof_mod_close(zof_any ref) {
	Mod* mod = (Mod*)ref;
	dlclose(mod->lib);
}

zof_type zof_mod_type(void) {
	static zof_type type = NULL;
	if (!type) {
		zof_type_info info;
		info.name = "zof_mod";
		info.close = zof_mod_close;
		type = zof_type_new(&info);
	}
	return type;
}

zof_mod zof_mod_new(zof_str uri) {
	void* lib = dlopen(uri, RTLD_NOW);
	if (!lib) {
		// TODO Store error instead of printing.
		fprintf(stderr, "%s\n", dlerror());
		return NULL;
	}
	Mod* mod = (Mod*)malloc(sizeof(Mod));
	mod->type = zof_mod_type();
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
		fprintf(stderr, "%s\n", error);
		return zof_false;
	}
	return sim_init(mod, sim);
}

}
