#ifndef hum_bot_h
#define hum_bot_h

#include "hum.h"
#include <vector>

using namespace std;

namespace hum {

struct Arm;
struct Finger;
struct Hand;
struct Head;
struct Bot;
struct Torso;
struct WheeledBase;

struct Arm: Part {

	Arm(int side);

	Hand* hand;
	zofJoint elbowToLower;
	zofJoint shoulderToTorso;
	zofJoint shoulderToUpper;
	zofJoint upperToElbow;

};

struct Bot: Part {

	Bot();

	vector<Arm*> arms;
	WheeledBase* base;
	Head* head;
	Torso* torso;

};

struct Finger: Part {

	Finger(int side, int phalanxCount);

	zofJoint spreadToHand;
	vector<zofJoint> curls;

};

struct Head: Part {

	Head();

	zofJoint neckToSkull;
	zofJoint neckToTorso;

};

struct Hand: Part {

	Hand(int side);

	vector<Finger*> fingers;
	zofJoint wristToArm;
	zofJoint palmToThumbTwist;
	Finger* thumb;

};

struct Torso: Part {

	Torso();

	zofJoint chestToAbdomen;

};

struct WheeledBase: Part {

	WheeledBase();

	/**
	 * So far, our tires are simple, but it's so easy to imagine them as
	 * independent parts, and they could have caps or whatnot, too.
	 *
	 * TODO Introduce size parameters and so on.
	 */
	zofPart makeWheel(int side);

	/**
	 * Sets the positional (translational) and rotational (angular)
	 * velocities of the base.
	 */
	void setVel(zofM pos, zofRat rot);

	zofJoint hipsToTorso;
	vector<zofJoint> hipsToWheels;

};

}

#endif
