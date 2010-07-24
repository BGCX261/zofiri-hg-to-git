#include "house.h"

namespace hum {

void House::build(zofSim sim) {
	zofPart wall1 = zofPartNewBox("wall1", zofV3(0.075,1.25,2.5));
	zofM3 wall1Radii = zofPartRadii(wall1);
	zofPartJointPut(wall1, zofJointNew("wall2", zofV3(0,0,wall1Radii.vals[0]-wall1Radii.vals[2])));
	zofPart wall2 = zofPartNewBox("wall2", zofV3(0.075,1.25,2.5));
	zofM3 wall2Radii = zofPartRadii(wall1);
	zofPartJointPut(wall2, zofJointNewEx("wall1", zofV3(0,0,wall2Radii.vals[2]-wall2Radii.vals[0]), zofV4(0,1,0,-0.5)));
	zofPartAttach(wall1, wall2);
	zofPart walls = zofPartNewGroup("walls", wall1);
	zofPartMaterialPut(walls, zofMaterialNew(0xFFFFE0A0,1));
	zofPartPosPut(walls, zofV3(2,-zofPartExtents(walls).min.vals[1],0));
	zofSimPartAdd(sim, walls);
}

}
