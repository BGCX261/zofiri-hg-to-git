#include "control.h"

namespace hum {

void update(zofSim sim, Humanoid* humanoid) {
	static int i = 0;
	static const int max = 400;
	i = (i + 1) % max;
	zofJointPosPut(humanoid->head->neckToSkull, 0);
	zofJointPosPut(humanoid->head->neckToTorso, 0);//i < max/2 ? 0.25 : -0.25);
	zofJointPosPut(humanoid->torso->chestToAbdomen, 0);
	// Left arm.
	zofJointPosPut(humanoid->arms[0]->shoulderToTorso, 0.25);
	zofJointPosPut(humanoid->arms[0]->shoulderToUpper, 0);
	zofJointPosPut(humanoid->arms[0]->upperToElbow, -0.25);//i < max/2 ? 0.5 : -0.5);
	zofJointPosPut(humanoid->arms[0]->elbowToLower, 0.25);
	zofJointPosPut(humanoid->arms[0]->hand->wristToArm, 0.25);
	zofJointPosPut(humanoid->arms[0]->hand->palmToThumbTwist, 0);//i < max/2 ? 1 : -1);
	// Right arm.
	zofJointPosPut(humanoid->arms[1]->shoulderToTorso, 0.25);
	zofJointPosPut(humanoid->arms[1]->shoulderToUpper, 0);
	zofJointPosPut(humanoid->arms[1]->upperToElbow, 0.25);
	zofJointPosPut(humanoid->arms[1]->elbowToLower, 0.25);
	zofJointPosPut(humanoid->arms[1]->hand->wristToArm, -0.5);
	zofJointPosPut(humanoid->arms[1]->hand->palmToThumbTwist, 0);//i < max/2 ? 1 : -1);
	// Base.
	zofJointPosPut(humanoid->base->hipsToTorso, 0);//i < max/2 ? -0.25 : 0.25);
	//zofJointVelPut(humanoid->base->hipsToWheels[0], 0.15);
	//zofJointVelPut(humanoid->base->hipsToWheels[1], 0.15);
	zofJointPosPut(humanoid->base->hipsToWheels[0], 0);
	zofJointPosPut(humanoid->base->hipsToWheels[1], 0);
}

}
