#include "control.h"
#include "house.h"
#include <iostream>

using namespace hum;
using namespace std;

extern "C" {

void humUpdate(zofSim sim, zofAny data) {
	reinterpret_cast<Controller*>(data)->update();
}

zofModExport zofBool zofSimInit(zofMod mod, zofSim sim) {
	// House.
	House* house = new House;
	zofSimPartAdd(sim, house->zof);
	// Bot.
	Bot* bot = new Bot;
	zofExtentsM3 botExtents = zofPartExtents(bot->zof);
	zofPartPosPut(bot->zof, zofV3(0,-botExtents.min.vals[1],0));
	zofSimPartAdd(sim, bot->zof);
	// Controller.
	Controller* controller = new Controller(bot);
	zofM3 counterPos = zofPartPos(house->countertopSoutheast);
	zofExtentsM3 counterBounds = zofPartBounds(house->countertopSoutheast);
	controller->targetPos = zofV3(
		// The counter AABB only works if the counter is axis-aligned, too. For now, it is.
		counterBounds.min.vals[0] - botExtents.max.vals[2] - 0.1,
		0,
		counterPos.vals[2]
	);
	controller->targetRot = zofV4(0,1,0,0.5); // Facing east (positive X).
	zofSimUpdaterAdd(sim, humUpdate, controller);
	// Done.
	return zofTrue;
}

}
