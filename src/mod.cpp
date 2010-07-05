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
	zofString uri;

};

}

using namespace zof;

extern "C" {

zofMod zofModNew(zofString uri) {
	void* lib;
	// TODO Real uri handling.
	// TODO Store error instead of printing. <-- ??
	#ifdef _WIN32
		// Need absolute path to avoid searching system in Windows.
		TCHAR absPath[4096] = TEXT("");
		if (!GetFullPathName(TEXT(uri), 4096, absPath, 0)) {
			cerr << "GetFullPathName error for " << uri << ": " << GetLastError() << endl;
		}
		//cerr << "Loading " << absPath << endl;
		lib = LoadLibrary(absPath);
		if (!lib) {
			cerr << "LoadLibrary error: " << GetLastError() << endl;
			return NULL;
		}
	#else
		// Might want to guarantee absolute here, too, in case no '/' or whatnot.
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
	return (zofMod)mod;
}

zofString zofModUri(zofMod mod) {
	return ((Mod*)mod)->uri;
}

zofBool zofModSimInit(zofMod mod, zofSim sim) {
	// TODO Move these guts to a Mod method.
	Mod* mod_struct = (Mod*)mod;
	zofBool (*simInit)(zofMod,zofSim);
	// C supposedly likes the former, but g++ likes the latter.
	//*(void**)(&zofSimInit) = dlsym(lib, "zofSimInit");
	#ifdef _WIN32
		simInit = (zofBool(*)(zofMod,zofSim))GetProcAddress(reinterpret_cast<HMODULE>(mod_struct->lib), "zofSimInit");
		if (!simInit) {
			cerr << "GetProcAddress error: " << GetLastError() << endl;
			return zofFalse;
		}
	#else
		// Lock something around dlerror calls?
		dlerror();
		simInit = (zofBool(*)(zofMod,zofSim))dlsym(mod_struct->lib, "zofSimInit");
		char* error;
		if ((error = dlerror()) != NULL) {
			cerr << error << endl;
			return zofFalse;
		}
	#endif
	return simInit(mod, sim);
}

}
