#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <zof.h>

extern "C" {

struct zof_mod_struct {
	zof_type* meta;
	void* lib;
	zof_str uri;
};

void zof_mod_close(zof_any ref) {
	zof_mod_struct* mod = (zof_mod_struct*)ref;
	dlclose(mod->lib);
}

zof_type* zof_mod_type() {
	static zof_type* type = NULL;
	if (!type) {
		type = (zof_type*)malloc(sizeof(zof_type*));
		type->close = zof_mod_close;
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
	zof_mod_struct* mod = (zof_mod_struct*)malloc(sizeof(zof_mod_struct));
	mod->meta = zof_mod_type();
	mod->lib = lib;
	// TODO Copy the uri first?
	mod->uri = uri;
	return (zof_mod)mod;
}

zof_str zof_mod_uri(zof_mod mod) {
	return ((zof_mod_struct*)mod)->uri;
}

zof_bool zof_mod_sim_init(zof_mod mod, zof_sim sim) {
	zof_mod_struct* mod_struct = (zof_mod_struct*)mod;
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
