#ifdef _WIN32
	#include <windows.h>
#else
	#include <dlfcn.h>
#endif

#include <iostream>
#include <stdlib.h>
#include <zofiri.h>

using namespace std;

namespace zof {

struct Mod: Any {

	virtual ~Mod() {
		#ifdef _WIN32
			FreeLibrary(reinterpret_cast<HMODULE>(lib));
		#else
			dlclose(lib);
		#endif
	}

	void* lib;
	zof_str uri;

};

}

using namespace zof;

extern "C" {

zof_mod zof_mod_new(zof_str uri) {
	void* lib;
	// TODO Real uri handling.
	// TODO Store error instead of printing. <-- ??
	#ifdef _WIN32
		lib = LoadLibrary(TEXT(uri));
		if (!lib) {
			cerr << "LoadLibrary error: " << GetLastError() << endl;
			return NULL;
		}
	#else
		lib = dlopen(uri, RTLD_NOW);
		if (!lib) {
			// TODO Store error instead of printing.
			cerr << dlerror() << endl;
			return NULL;
		}
	#endif
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
	// TODO Move these guts to a Mod method.
	Mod* mod_struct = (Mod*)mod;
	zof_bool (*sim_init)(zof_mod,zof_sim);
	// C supposedly likes the former, but g++ likes the latter.
	//*(void**)(&sim_init) = dlsym(lib, "sim_init");
	#ifdef _WIN32
		sim_init = (zof_bool(*)(zof_mod,zof_sim))GetProcAddress(reinterpret_cast<HMODULE>(mod_struct->lib), "sim_init");
		if (!sim_init) {
			cerr << "GetProcAddress error: " << GetLastError() << endl;
			return zof_false;
		}
	#else
		// Lock something around dlerror calls?
		dlerror();
		sim_init = (zof_bool(*)(zof_mod,zof_sim))dlsym(mod_struct->lib, "sim_init");
		char* error;
		if ((error = dlerror()) != NULL) {
			cerr << error << endl;
			return zof_false;
		}
	#endif
	return sim_init(mod, sim);
}

}
