#include "control.h"
#include <zof.h>

using namespace hum;
using namespace std;

extern "C" {

void humUpdate(zofSim sim, zofAny data) {
	update(sim, reinterpret_cast<Humanoid*>(data));
}

zofModExport zofBool zofSimInit(zofMod mod, zofSim sim) {
	Humanoid* humanoid = new Humanoid();
	zofPartPosPut(humanoid->zof, zofV3(-0.2,-zofPartExtents(humanoid->zof).min.vals[1],0.2));
	zofSimPartAdd(sim, humanoid->zof);
	zofSimUpdaterAdd(sim, humUpdate, humanoid);
	return zofTrue;
}

}
