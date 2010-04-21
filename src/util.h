#ifndef zof_util_h
#define zof_util_h

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
Num sign(Num n) {
	return n ? (n > 0 ? 1 : -1) : 0;
}

}

#endif
