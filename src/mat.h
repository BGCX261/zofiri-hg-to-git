#ifndef zof_mat_h
#define zof_mat_h

#include <stdlib.h>

// For math and matrix purposes.

/*
typedef struct {
	zof_meta meta;
	zof_int size_col;
	zof_int size_row;
	zof_int stride_col;
	zof_int stride_row;
	zof_int* values;
} zof_mat_i;

typedef struct {
	zof_meta meta;
	zof_mat_int colors;
	zof_mat coords;
	zof_mat normals;
	// 1xN for strip, 3xN for array.
	zof_mat_i triangles;
} zof_mesh_struct;
*/

namespace zof {

template<typename Num>
Num pi(Num scale = 1.0) {
	return scale * 3.14159265358979323846;
}

template<typename Num>
Num radToDeg(Num rad) {
	return 180 * rad / pi<Num>();
}

template<typename Num>
Num random(Num scale = 1) {
	return scale * Num(rand()) / RAND_MAX;
}

template<typename Num>
Num sign(Num n) {
	return n ? (n > 0 ? 1 : -1) : 0;
}

}

#endif
