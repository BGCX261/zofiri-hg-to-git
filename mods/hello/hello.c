#include <stdio.h>
#include "zof.h"

zofModExport zofBool zofSimInit(zofMod mod, zofSim sim) {
	zofPart box1, box2;
	zofJoint joint1;
	printf("Hello from %s!\n", zofModUri(mod));
	// Box 1.
	box1 = zofPartNewBox("box1", zofV3(0.1,0.025,0.1));
	zofPartPosPut(box1, zofV3(-0.2,1,0.2));
	joint1 = zofJointNewEx(
		"box2",
		zofPartEndPos(box1, zofV3(-1,0,0)),
		zofV4(0,1,0,0.5)
	);
	zofPartMaterialPut(box1, zofMaterialNew(0xFFFF8000,1));
	// TODO Joint limits.
	zofPartJointPut(box1, joint1);
	// Box 2.
	box2 = zofPartNewBox("box2", zofV3(0.08,0.04,0.08));
	zofPartJointPut(box2, zofJointNewEx(
		"box1",
		zofPartEndPos(box2, zofV3(0,1,0)),
		zofV4(1,0,0,-0.5)
	));
	zofPartMaterialPut(box2, zofMaterialNew(0xFF800080,1));
	// Attach the boxes.
	zofPartAttach(box1, box2);
	// Add them.
	zofSimPartAdd(sim, box1);
	return zofTrue;
}
