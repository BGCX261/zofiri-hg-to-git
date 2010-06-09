#include "zof.h"
#include <stdlib.h>

extern "C" {

struct zof_type_struct {
	zof_type type;
	zof_type_info info;
};

zof_type zof_type_type(void) {
	// TODO Thread safety?
	static zof_type type = NULL;
	if (!type) {
		zof_type_info info;
		info.name = "zof_type";
		info.close = NULL;
		// Manual type creation here to avoid infinite recursion.
		// Could make a separate helper function instead.
		zof_type_struct* type_struct = (zof_type_struct*)malloc(sizeof(zof_type_struct));
		type = (zof_type)type_struct;
		type_struct->info = info;
		type_struct->type = type;
	}
	return type;
}

zof_type zof_type_new(zof_type_info* info) {
	// Make a convenience for malloc.
	zof_type_struct* type = (zof_type_struct*)malloc(sizeof(zof_type_struct));
	type->info = *info;
	type->type = zof_type_type();
	return (zof_type)type;
}

zof_ref_close zof_type_ref_close(zof_type type) {
	return reinterpret_cast<zof_type_struct*>(type)->info.close;
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
