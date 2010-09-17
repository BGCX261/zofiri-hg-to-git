#ifndef hum_house_h
#define hum_house_h

#include "hum.h"

using namespace std;

namespace hum {

/**
 * TODO Break into separate rooms for access to per-room fixtures.
 */
struct House: Part {

	House();

	void placeItems(zofSim sim);

	zofPart countertopSoutheast;

};

/**
 * Like a salt or pepper shaker, or a spice container. Cylindrical.
 *
 * TODO In the end, I probably ought to support parts without specialized
 * TODO types. Such as for dynamically configured data (like from data files.)
 */
struct Shaker: Part {

	Shaker(zofColor lidColor, zofColor bottleColor);

};

/**
 * TODO Generic table/desk idea for use around the house.
 */
struct Table {

	Table();

};

}

#endif
