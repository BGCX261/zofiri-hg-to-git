#include "house.h"

namespace hum {

void House::build(zofSim sim) {
	zofM3 wallEastRadii = zofV3(0.075,1.25,2);
	zofPart wallEast = zofPartNewBox("wallEast", wallEastRadii);
	zofPartJointPut(wallEast, zofJointNew("wallSouth", zofV3(0,0,wallEastRadii.vals[0]-wallEastRadii.vals[2])));
	zofM3 wallSouthRadii = wallEastRadii;
	zofPart wallSouth = zofPartNewBox("wallSouth", wallSouthRadii);
	zofPartJointPut(wallSouth, zofJointNewEx("wallEast", zofV3(0,0,wallSouthRadii.vals[2]-wallSouthRadii.vals[0]), zofV4(0,1,0,-0.5)));
	zofPartAttach(wallEast, wallSouth);
	// Cabinet southeast lower.
	zofM3 cabinetSoutheastLowerRadii = zofV3(0.3,0.45,1);
	zofPart cabinetSoutheastLower = zofPartNewBox("cabinet", cabinetSoutheastLowerRadii);
	zofPartMaterialPut(cabinetSoutheastLower, zofMaterialNew(0xFF806040,1));
	zofPartJointPut(cabinetSoutheastLower, zofJointNew("wallEast", zofPartEndPos(cabinetSoutheastLower,zofV3(1,-1,-1))));
	// Countertop southeast.
	zofPart countertopSoutheast = zofPartNewBox("countertop", zofV3(cabinetSoutheastLowerRadii.vals[0]+0.01,0.02,cabinetSoutheastLowerRadii.vals[2]));
	zofPartMaterialPut(countertopSoutheast, zofMaterialNew(0xFFFFFFFF,1));
	zofPartJointPut(countertopSoutheast, zofJointNew("cabinet", zofPartEndPos(countertopSoutheast,zofV3(1,-1,-1))));
	zofPartJointPut(cabinetSoutheastLower, zofJointNew("countertop", zofPartEndPos(cabinetSoutheastLower, zofV3(1,1,-1))));
	zofPartAttach(cabinetSoutheastLower, countertopSoutheast);
	// Counter southeast.
	zofPartJointPut(wallEast, zofJointNew("counterSoutheast", zofV3(-wallEastRadii.vals[0],-wallEastRadii.vals[1],2*wallEastRadii.vals[0]-wallEastRadii.vals[2])));
	zofPartAttach(wallEast, zofPartNewGroup("counterSoutheast", cabinetSoutheastLower));
	// Walls group.
	zofPart walls = zofPartNewGroup("walls", wallEast);
	zofPartMaterialPut(walls, zofMaterialNew(0xFFFFE0A0,1));
	zofPartPosPut(walls, zofV3(2,-zofPartExtents(walls).min.vals[1],0));
	zofSimPartAdd(sim, walls);
}

}
