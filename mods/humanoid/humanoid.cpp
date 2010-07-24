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
	Humanoid* humanoid = new Humanoid();
	zofPartPosPut(humanoid->zof, zofV3(0,-zofPartExtents(humanoid->zof).min.vals[1],0));
	zofSimPartAdd(sim, humanoid->zof);
	Controller* controller = new Controller(humanoid);
	zofSimUpdaterAdd(sim, humUpdate, controller);
	//cerr << "Head at: " << zofPartPos(humanoid->head->zof).vals[1] << endl;
	//zofExtentsM3 extents = zofPartExtents(humanoid->zof);
	//cerr << "Height: " << (extents.max.vals[1] - extents.min.vals[1]) << endl;
	return zofTrue;
}

}
