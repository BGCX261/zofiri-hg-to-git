#include "control.h"
#include <iostream>
#include <math.h>

namespace hum {

Controller::Controller(Humanoid* humanoid) {
	this->humanoid = humanoid;
}

void Controller::setVel(double pos, double rot) {
	zofJoint toWheelLeft = humanoid->base->hipsToWheels[0];
	zofJoint toWheelRight = humanoid->base->hipsToWheels[1];
	zofPart wheelLeft = zofJointPart(zofJointOther(toWheelLeft));
	zofPart wheelRight = zofJointPart(zofJointOther(toWheelRight));
	// The wheel radius is easy.
	zofNum radius = zofPartRadii(wheelLeft).vals[0];
	// Calculate the spread between the wheels.
	zofM3 posLeft = zofPartPos(wheelLeft);
	zofM3 posRight = zofPartPos(wheelRight);
	zofNum dX = posRight.vals[0] - posLeft.vals[0];
	zofNum dY = posRight.vals[1] - posLeft.vals[1];
	zofNum dZ = posRight.vals[2] - posLeft.vals[2];
	zofNum dist = sqrt(dX*dX + dY*dY + dZ*dZ) / 2;
	// A bit of algebra for individual rotation velocities.
	zofNum rotLeft = (dist * rot + pos) / radius;
	zofNum rotRight = 2*pos/radius - rotLeft;
	// Then set the velocities, converting from radians to rats.
	zofJointVelPut(toWheelLeft, rotLeft/(2*zofPi));
	zofJointVelPut(toWheelRight, rotRight/(2*zofPi));
}

void Controller::update() {
	static int i = 0;
	static const int max = 400;
	i = (i + 1) % max;
	zofJointPosPut(humanoid->head->neckToSkull, 0);
	zofJointPosPut(humanoid->head->neckToTorso, 0);//i < max/2 ? 0.25 : -0.25);
	zofJointPosPut(humanoid->torso->chestToAbdomen, 0);
	// Arms.
	for (int a = 0; a < 2; a++) {
		zofJointPosPut(humanoid->arms[a]->shoulderToTorso, 0.25);
		zofJointPosPut(humanoid->arms[a]->shoulderToUpper, 0);
		zofJointPosPut(humanoid->arms[a]->upperToElbow, -0.25);//i < max/2 ? 0.5 : -0.5);
		zofJointPosPut(humanoid->arms[a]->elbowToLower, 0.25);
		Hand* hand = humanoid->arms[a]->hand;
		zofJointPosPut(hand->wristToArm, 0.25);
		zofJointPosPut(hand->palmToThumbTwist, 0);//i < max/2 ? 1 : -1);
		struct FingerController {
			static void updateFinger(Finger* finger, int side) {
				// TODO For the finger control, it seems the arms throw them too hard.
				// TODO With a higher velocity for position restitution, we could perhaps fight that.
				if (side) {
					// Vel instead of pos because Bullet hinge pos control doesn't fight gravity too well.
					// TODO Our own PD controller might help a little more.
					zofJointVelPut(finger->spreadToHand, side);
				} else {
					// But use pos for zero because it will at least fight slippage a little.
					zofJointPosPut(finger->spreadToHand, side);
				}
				// Curl the fingers in a bit.
				for (vector<zofJoint>::iterator c = finger->curls.begin(); c != finger->curls.end(); c++) {
					zofJoint curl = *c;
					zofJointPosPut(curl, 0.15);
				}
			}
		};
		FingerController::updateFinger(hand->thumb, 0);
		for (vector<Finger*>::size_type f = 0; f < hand->fingers.size(); f++) {
			FingerController::updateFinger(hand->fingers[f], static_cast<int>(f) - 1);
		}
	}
	// Base.
	zofJointPosPut(humanoid->base->hipsToTorso, 0);//i < max/2 ? -0.25 : 0.25);
	zofJointPosPut(humanoid->base->hipsToWheels[0], 0);
	zofJointPosPut(humanoid->base->hipsToWheels[1], 0);
	//zofJointVelPut(humanoid->base->hipsToWheels[0], 0.15);
	//zofJointVelPut(humanoid->base->hipsToWheels[1], 0.15);
	setVel(0.25, 0.5);
}

}
