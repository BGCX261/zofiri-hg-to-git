#include "zofiri.h"
#include <stdlib.h>

namespace zof {

Any::~Any() {
	// Nothing to do by default.
}

/**
 * TODO Should I retain this for mods to interop better?
 */
struct Type {
	zof_type type;
	zof_type_info info;
};

}

using namespace zof;

extern "C" {

zof_num zof_num_max(zof_num a, zof_num b) {
	return a > b ? a : b;
}

zof_num zof_num_min(zof_num a, zof_num b) {
	return a < b ? a : b;
}

void zof_ref_free(zof_any ref) {
	delete reinterpret_cast<Any*>(ref);
}

zof_type zof_type_type(void) {
	// TODO Thread safety?
	static zof_type type = NULL;
	if (!type) {
		zof_type_info info;
		info.name = const_cast<zof_str>("zof_type");
		info.close = NULL;
		// Manual type creation here to avoid infinite recursion.
		// Could make a separate helper function instead.
		Type* type_struct = new Type;
		type = (zof_type)type_struct;
		type_struct->info = info;
		type_struct->type = type;
	}
	return type;
}

zof_type zof_type_new(zof_type_info* info) {
	Type* type = new Type;
	type->info = *info;
	type->type = zof_type_type();
	return (zof_type)type;
}

zof_ref_close zof_type_ref_close(zof_type type) {
	return reinterpret_cast<Type*>(type)->info.close;
}

zof_vec4 zof_xyz(zof_num x, zof_num y, zof_num z) {
	return zof_xyzw(x,y,z,0);
}

zof_vec4 zof_xyzw(zof_num x, zof_num y, zof_num z, zof_num w) {
	zof_vec4 vec;
	vec.vals[0] = x;
	vec.vals[1] = y;
	vec.vals[2] = z;
	vec.vals[3] = w;
	return vec;
}

}
