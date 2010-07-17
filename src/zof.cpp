#include "zofiri.h"
#include <limits>
#include <stdlib.h>

using namespace std;

namespace zof {

Any::~Any() {
	// Nothing to do by default.
}

/**
 * TODO Should I retain this for mods to interop better?
 */
struct Type {
	zofType type;
	zofTypeInfo info;
};

}

using namespace zof;

extern "C" {

zofNum zofNan() {
	return numeric_limits<zofNum>::quiet_NaN();
}

zofNum zofInf() {
	return numeric_limits<zofNum>::infinity();
}

zofNum zofNumMax(zofNum a, zofNum b) {
	return a > b ? a : b;
}

zofNum zofNumMin(zofNum a, zofNum b) {
	return a < b ? a : b;
}

void zofRefFree(zofAny ref) {
	delete reinterpret_cast<Any*>(ref);
}

zofType zof_type_type(void) {
	// TODO Thread safety?
	static zofType type = NULL;
	if (!type) {
		zofTypeInfo info;
		info.name = const_cast<zofString>("zof_type");
		info.close = NULL;
		// Manual type creation here to avoid infinite recursion.
		// Could make a separate helper function instead.
		Type* type_struct = new Type;
		type = (zofType)type_struct;
		type_struct->info = info;
		type_struct->type = type;
	}
	return type;
}

zofType zofTypeNew(zofTypeInfo* info) {
	Type* type = new Type;
	type->info = *info;
	type->type = zof_type_type();
	return (zofType)type;
}

zofRefClose zofTypeRefClose(zofType type) {
	return reinterpret_cast<Type*>(type)->info.close;
}

zofVec4 zofXyz(zofNum x, zofNum y, zofNum z) {
	return zofXyzw(x,y,z,0);
}

zofVec4 zofXyzw(zofNum x, zofNum y, zofNum z, zofNum w) {
	zofVec4 vec;
	vec.vals[0] = x;
	vec.vals[1] = y;
	vec.vals[2] = z;
	vec.vals[3] = w;
	return vec;
}

}
