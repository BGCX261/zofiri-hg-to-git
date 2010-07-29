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
	House::build(sim);
	Bot* bot = new Bot();
	zofPartPosPut(bot->zof, zofV3(0,-zofPartExtents(bot->zof).min.vals[1],0));
	zofSimPartAdd(sim, bot->zof);
	Controller* controller = new Controller(bot);
	zofSimUpdaterAdd(sim, humUpdate, controller);
	//cerr << "Head at: " << zofPartPos(bot->head->zof).vals[1] << endl;
	//zofExtentsM3 extents = zofPartExtents(bot->zof);
	//cerr << "Height: " << (extents.max.vals[1] - extents.min.vals[1]) << endl;
	return zofTrue;
}

}
