#ifndef hum_hum_h
#define hum_hum_h

#include <zof.h>

namespace hum {

/**
 * Somewhat repeats the "Part" concept from zof implementation, but now
 * it is built on (instead of under) the published C API.
 */
struct Part {

	/**
	 * Just a convenience for zofPartAttach.
	 */
	void attach(Part* part, bool swap = false);

	zofPart zof;

};

}

#endif
