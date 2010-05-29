#ifndef zof_mat_h
#define zof_mat_h

#include <stdlib.h>

// For math and matrix purposes.

namespace zof {

template<typename Num>
Num pi(Num scale = 1) {
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
