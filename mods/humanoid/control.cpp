#include "control.h"
#include <iostream>
#include <math.h>

namespace hum {

void driveToTarget(Controller* controller);

Controller::Controller(Bot* bot) {
	this->bot = bot;
}

void Controller::update() {
	static int i = 0;
	static const int max = 400;
	i = (i + 1) % max;
	zofJointPosPut(bot->head->neckToSkull, 0);
	zofJointPosPut(bot->head->neckToTorso, 0);//i < max/2 ? 0.25 : -0.25);
	zofJointPosPut(bot->torso->chestToAbdomen, 0);
	// Arms.
	for (int a = 0; a < 2; a++) {
		zofJointPosPut(bot->arms[a]->shoulderToTorso, 0.25);
		zofJointPosPut(bot->arms[a]->shoulderToUpper, 0);
		zofJointPosPut(bot->arms[a]->upperToElbow, -0.25);//i < max/2 ? 0.5 : -0.5);
		zofJointPosPut(bot->arms[a]->elbowToLower, 0.25);
		Hand* hand = bot->arms[a]->hand;
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
	zofJointPosPut(bot->base->hipsToTorso, 0);//i < max/2 ? -0.25 : 0.25);
	//zofJointPosPut(bot->base->hipsToWheels[0], 0);
	//zofJointPosPut(bot->base->hipsToWheels[1], 0);
	//zofJointVelPut(bot->base->hipsToWheels[0], 0.15);
	//zofJointVelPut(bot->base->hipsToWheels[1], 0.15);
	driveToTarget(this);
}

void driveToTarget(Controller* controller) {
	zofM posVel = 0;
	zofRat rotVel = 0;
	zofM3 botPos = zofPartPos(controller->bot->zof);
	zofM3Rat botRot = zofPartRot(controller->bot->zof);
	zofM dX = controller->goalPos.vals[0] - botPos.vals[0];
	zofM dZ = controller->goalPos.vals[2] - botPos.vals[2];
	zofM dist = sqrt(dX*dX + dZ*dZ);
	posVel = min(dist, 0.25);
	if (dist > 0.2) {
		zofRat ratsToGoal = atan2(dX,dZ) / zofPi;
		// TODO Verify positive Y axis?
		// TODO Watch for angle wrap-arounds near -1/1.
		zofRat dAngle = ratsToGoal - botRot.vals[3];
		//cerr << "dAngle " << dAngle << " for " << dX << ", " << dZ << endl;
		if (dAngle > 0.1) {
			// If angle off too much, don't drive.
			// TODO Get more sophisticated about arcs.
			posVel = 0;
		}
		rotVel = dAngle;
	} else {
		posVel = 0;
		rotVel = controller->goalRot.vals[3] - botRot.vals[3];
		//cerr << "At goal: dRot " << rotVel << endl;
	}
	controller->bot->base->setVel(posVel, rotVel);
}

}
