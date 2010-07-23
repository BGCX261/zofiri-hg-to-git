#include "hum.h"

namespace hum {

void Part::attach(Part* part, bool swap) {
	zofPartAttachSwap(zof, part->zof, swap ? zofTrue : zofFalse);
}

}
