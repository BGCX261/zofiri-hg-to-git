#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <zof.h>

using namespace std;

namespace hum {

struct Part {

	/**
	 * Just a convenience for zofPartAttach.
	 */
	void attach(Part* part, bool swap = false);

	zofPart zof;

};

struct Arm;
struct Finger;
struct Hand;
struct Head;
struct Humanoid;
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

struct Humanoid: Part {

	Humanoid();

	vector<Arm*> arms;
	WheeledBase* base;
	Head* head;
	Torso* torso;

};

struct Torso: Part {

	Torso();

	zofJoint chestToAbdomen;

};

struct WheeledBase: Part {

	WheeledBase();

	zofJoint hipsToTorso;
	vector<zofJoint> hipsToWheels;

};

/**
 * So far, our tires are simple, but it's so easy to imagine them as
 * independent parts, and they could have caps or whatnot, too.
 *
 * TODO Introduce size parameters and so on.
 */
zofPart humWheelNew(void);

extern "C" void update(zofSim sim, zofAny data);

}

using namespace hum;

extern "C" zofModExport zofBool zofSimInit(zofMod mod, zofSim sim) {
	// TODO Obviously need more to the humanoid than this.
	// TODO Especially want group parts.
	Humanoid* humanoid = new Humanoid();
	zofPartPosPut(humanoid->zof, zofV3(-0.2,-zofPartExtents(humanoid->zof).min.vals[1],0.2));
	zofSimPartAdd(sim, humanoid->zof);
	zofSimUpdaterAdd(sim, update, humanoid);
	return zofTrue;
}

namespace hum {

void Part::attach(Part* part, bool swap) {
	zofPartAttachSwap(zof, part->zof, swap ? zofTrue : zofFalse);
}

Arm::Arm(int side) {
	zofPart elbow, lower, shoulder, upper;
	zofJoint elbowToUpper, lowerToElbow, lowerToHand, upperToShoulder;
	// Shoulder.
	shoulder = zofPartNewCapsule("shoulder", 0.05, 0);
	shoulderToTorso = zofJointNew("torso", zofV3(0,0,0));
	zofJointLimitsRotPut(shoulderToTorso, zofV3(0, 0, -0.25), zofV3(0, 0, 1));
	zofPartJointPut(shoulder, shoulderToTorso);
	// Upper.
	upper = zofPartNewCapsule("upper", 0.03, 0.05);
	shoulderToUpper = zofJointNew("upper", zofV3(0,0,0));
	zofJointLimitsRotPut(shoulderToUpper, zofV3(-0.5,0,0), zofV3(1,0,0));
	zofPartJointPut(shoulder, shoulderToUpper);
	upperToShoulder = zofJointNew("shoulder", zofCapsuleEndPos(zofPartCapsule(upper), 0.5));
	zofPartJointPut(upper, upperToShoulder);
	zofPartAttach(shoulder, upper);
	// Elbow. Needed because multiple DOFs without it are causing trouble.
	elbow = zofPartNewCapsule("elbow", 0.03, 0.01);
	upperToElbow = zofJointNew("elbow", zofCapsuleEndPos(zofPartCapsule(upper), -1));
	zofJointLimitsRotPut(upperToElbow, zofV3(0,-0.5,0), zofV3(0,0.5,0));
	zofPartJointPut(upper, upperToElbow);
	elbowToUpper = zofJointNew("upper", zofCapsuleEndPos(zofPartCapsule(elbow), 0.2));
	zofPartJointPut(elbow, elbowToUpper);
	zofPartAttach(upper, elbow);
	// Lower.
	lower = zofPartNewCapsule("lower", 0.03, 0.05);
	elbowToLower = zofJointNew("lower", zofCapsuleEndPos(zofPartCapsule(elbow), -0.5));
	zofJointLimitsRotPut(elbowToLower, zofV3(0,0,0), zofV3(0.8,0,0));
	zofPartJointPut(elbow, elbowToLower);
	lowerToElbow = zofJointNew("elbow", zofCapsuleEndPos(zofPartCapsule(lower), 0.5));
	zofPartJointPut(lower, lowerToElbow);
	zofPartAttach(elbow, lower);
	// Hand joints.
	lowerToHand = zofJointNew(side < 0 ? "handLeft" : "handRight", zofCapsuleEndPos(zofPartCapsule(lower), -1));
	zofJointLimitsRotPut(lowerToHand, zofV3(0, -(side < 0 ? 0.5 : 0.7), 0), zofV3(0, side < 0 ? 0.7 : 0.5, 0));
	zofPartJointPut(lower, lowerToHand);
	// Arm.
	zof = zofPartNewGroup(side < 0 ? "armLeft" : "armRight", shoulder);
	// Actually attach hand last since we want it separate from the arm group.
	attach(hand = new Hand(side));
}

Finger::Finger(int side, int phalanxCount) {
	zofPart current, spread;
	// Spread.
	// TODO Could 6-DOF avoid this? Or do I still fear 6-DOF instability?
	spread = zofPartNewCapsule("spread", 0.01, 0);
	spreadToHand = zofJointNew("hand", zofCapsuleEndPos(zofPartCapsule(spread), 0.5));
	zofJointLimitsRotPut(spreadToHand, zofV3(-0.2,0,0), zofV3(0.2,0,0));
	zofPartJointPut(spread, spreadToHand);
	current = spread;
	for (int p = 0; p < phalanxCount; p++) {
		zofPart next;
		zofJoint currentToNext, nextToCurrent;
		// TODO Number them?
		next = zofPartNewCapsule("phalanx", 0.01, 0.006);
		currentToNext = zofJointNew("next", zofCapsuleEndPos(zofPartCapsule(current), -0.5));
		curls.push_back(currentToNext);
		zofJointLimitsRotPut(currentToNext, zofV3(0, 0, side < 0 ? -0.5 : 0), zofV3(0, 0, side < 0 ? 0 : 0.5));
		zofPartJointPut(current, currentToNext);
		nextToCurrent = zofJointNew("prev", zofCapsuleEndPos(zofPartCapsule(next), 0.5));
		zofPartJointPut(next, nextToCurrent);
		zofJointAttach(currentToNext, nextToCurrent);
		current = next;
	}
	zof = zofPartNewGroup("fingerLeft", spread);
}

Hand::Hand(int side) {
	zofInt f;
	zofString fingerJointName;
	zofPart thumbTwist, wrist;
	zofJoint thumbTwistToThumb, thumbTwistToPalm;
	// Wrist.
	wrist = zofPartNewCapsule("wrist", 0.03, 0.005);
	wristToArm = zofJointNew(side < 0 ? "armLeft" : "armRight", zofCapsuleEndPos(zofPartCapsule(wrist), 0.3));
	zofPartJointPut(wrist, wristToArm);
	// Fingers.
	// TODO Better strings yet???
	fingerJointName = zofStringNewCopy("finger##");
	for (f = 0; f < 3; f++) {
		zofJoint wristToFinger;
		Finger* finger = new Finger(side, 3);
		fingers.push_back(finger);
		// Yes, the const_cast is evil, but it works for the moment.
		// TODO Move this to stl strings or some such.
		sprintf(const_cast<char*>(fingerJointName), "finger%d", f);
		wristToFinger = zofJointNew(fingerJointName, zofCapsuleEndPosEx(zofPartCapsule(wrist), 1, zofV3(-side,-2,2*(f-1)), 1));
		zofPartJointPut(wrist, wristToFinger);
		zofJointAttach(wristToFinger, zofPartJoint(finger->zof, "hand"));
	}
	// TODO zofFree?
	free(const_cast<char*>(fingerJointName));
	// Thumb.
	// TODO How does the side affect this mess?
	thumbTwist = zofPartNewCapsule("thumbTwist", 0.014, 0.004);
	palmToThumbTwist = zofJointNew("thumbTwist", zofCapsuleEndPosEx(zofPartCapsule(wrist), 1, zofV3(-side*1.5,0.7,1), 0));
	zofJointLimitsRotPut(palmToThumbTwist, zofV3(0, -0.25, 0), zofV3(0, 0.5, 0));
	zofPartJointPut(wrist, palmToThumbTwist);
	thumbTwistToPalm = zofJointNewEx("wrist", zofCapsuleEndPos(zofPartCapsule(thumbTwist),0.2), zofV4(0,0,1,side*0.5));
	zofPartJointPut(thumbTwist, thumbTwistToPalm);
	thumbTwistToThumb = zofJointNewEx(
		"thumb",
		zofCapsuleEndPosEx(zofPartCapsule(thumbTwist), -1, zofV3(side,2,0), -1),
		zofV4(0, 1, 0, -side*0.75)
	);
	zofPartJointPut(thumbTwist, thumbTwistToThumb);
	thumb = new Finger(side, 2);
	zofJointAttach(thumbTwistToThumb, zofPartJoint(thumb->zof, "hand"));
	zofPartAttachSwap(wrist, thumbTwist, side < 0 ? zofFalse : zofTrue);
	// Hand.
	zof = zofPartNewGroup(side < 0 ? "handLeft" : "handRight", wrist);
}

Head::Head() {
	zofPart eyeLeft, neck, skull;
	zofJoint eyeLeftToSkull, skullToEyeLeft, skullToNeck;
	// Skull.
	skull = zofPartNewCapsule("skull", 0.06, 0.01);
	//zofPartMaterialPut(skull, zofMaterialNew(0xFFA0A0A0, 0.1));
	skullToNeck = zofJointNewEx(
		"neck",
		zofCapsuleEndPos(zofPartCapsule(skull), -1),
		// TODO Rotate around Z for Y sideways? Or just make X the axis of rotation?
		zofV4(0,1,0,0)
	);
	zofPartJointPut(skull, skullToNeck);
	// Neck.
	neck = zofPartNewCapsule("neck", 0.04, 0);
	neckToTorso = zofJointNew("torso", zofCapsuleEndPos(zofPartCapsule(neck),-0.3));
	zofJointLimitsRotPut(neckToTorso, zofV3(0,-0.5,0), zofV3(0,0.5,0));
	zofPartJointPut(neck, neckToTorso);
	neckToSkull = zofJointNewEx(
		"skull",
		zofCapsuleEndPos(zofPartCapsule(neck), 0.3),
		// TODO Rotate around Z for Y sideways? Or just make X the axis of rotation?
		zofV4(0,1,0,0)
	);
	zofJointLimitsRotPut(neckToSkull, zofV3(-0.5,0,0), zofV3(0.5,0,0));
	zofPartJointPut(neck, neckToSkull);
	zofPartAttach(skull, neck);
	// Eyes.
	// TODO Consider making eyes a separately defined part.
	eyeLeft = zofPartNewCapsule("eyeLeft", 0.015, 0);
	zofPartMaterialPut(eyeLeft, zofMaterialNew(0xFF0060A0,0.001));
	eyeLeftToSkull = zofJointNew("skull", zofV3(0,0,0));
	zofPartJointPut(eyeLeft, eyeLeftToSkull);
	skullToEyeLeft = zofJointNew(
		"eyeLeft",
		zofCapsuleEndPosEx(zofPartCapsule(skull), 0.8, zofV3(-0.2,0,1), 1)
	);
	zofPartJointPut(skull, skullToEyeLeft);
	zofPartAttach(skull, eyeLeft);
	zofPartMirror(eyeLeft);
	// Head.
	zof = zofPartNewGroup("head", skull);
}

Humanoid::Humanoid() {
	// Torso, head, and base.
	torso = new Torso();
	torso->attach(head = new Head());
	torso->attach(base = new WheeledBase());
	// Arms.
	Arm* arm;
	torso->attach(arm = new Arm(-1));
	arms.push_back(arm);
	// TODO zofPartMirror(armLeft);
	torso->attach(arm = new Arm(1), true);
	arms.push_back(arm);
	// Humanoid.
	zof = zofPartNewGroup("humanoid", torso->zof);
	zofPartMaterialPut(zof, zofMaterialNew(0xFF808080,1));
}

Torso::Torso() {
	zofJoint abdomenToBase, abdomenToChest, chestToArmLeft, chestToHead;
	// Chest.
	zofPart chest = zofPartNewCapsule("chest", 0.1, 0.0725);
	chestToAbdomen = zofJointNew("abdomen", zofCapsuleEndPos(zofPartCapsule(chest),-0.5));
	zofPartJointPut(chest, chestToAbdomen);
	chestToHead = zofJointNew("head", zofCapsuleEndPos(zofPartCapsule(chest), 1));
	zofPartJointPut(chest, chestToHead);
	// Abdomen.
	zofPart abdomen = zofPartNewCapsule("abdomen", 0.08, 0.05);
	abdomenToChest = zofJointNew("chest", zofCapsuleEndPos(zofPartCapsule(abdomen),0.5));
	zofJointLimitsRotPut(abdomenToChest, zofV3(-0.05,0,0), zofV3(0.75,0,0));
	zofPartJointPut(abdomen, abdomenToChest);
	abdomenToBase = zofJointNew("base", zofCapsuleEndPos(zofPartCapsule(abdomen),-0.5));
	zofPartJointPut(abdomen, abdomenToBase);
	zofPartAttach(chest, abdomen);
	// Arm joint.
	// TODO It would make sense to mirror it here and then attach a mirrored arm.
	// TODO However, that's less convenient at the moment.
	// TODO It might even make sense to make shoulders part of the torso.
	chestToArmLeft = zofJointNew("armLeft", zofCapsuleEndPosEx(zofPartCapsule(chest), 1.4, zofV3(-1.5,1,0), 1));
	zofPartJointPut(chest, chestToArmLeft);
	zofPartJointPut(chest, zofJointNew("armRight", zofCapsuleEndPosEx(zofPartCapsule(chest), 1.4, zofV3(1.5,1,0), 1)));
	// Torso.
	zof = zofPartNewGroup("torso", chest);
}


extern "C" void update(zofSim sim, zofAny data) {
	static int i = 0;
	static const int max = 400;
	i = (i + 1) % max;
	Humanoid* humanoid = reinterpret_cast<Humanoid*>(data);
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
	zofJointPosPut(humanoid->arms[1]->shoulderToTorso, 0.5);
	zofJointPosPut(humanoid->arms[1]->shoulderToUpper, 0);
	zofJointPosPut(humanoid->arms[1]->upperToElbow, 0.25);
	zofJointPosPut(humanoid->arms[1]->elbowToLower, 0.25);
	zofJointPosPut(humanoid->arms[1]->hand->wristToArm, -0.5);
	zofJointPosPut(humanoid->arms[1]->hand->palmToThumbTwist, i < max/2 ? 1 : -1);
	// Base.
	zofJointPosPut(humanoid->base->hipsToTorso, 0);//i < max/2 ? -0.25 : 0.25);
	//zofJointVelPut(humanoid->base->hipsToWheels[0], 0.15);
	//zofJointVelPut(humanoid->base->hipsToWheels[1], 0.15);
	zofJointPosPut(humanoid->base->hipsToWheels[0], 0);
	zofJointPosPut(humanoid->base->hipsToWheels[1], 0);
}

zofPart humWheelNew(void) {
	zofPart wheel;
	zofJoint wheelToBody;
	wheel = zofPartNewCylinder("wheel", zofV3(0.18,0.03,0.18));
	zofPartMaterialPut(wheel, zofMaterialNew(0xFF202020, 20));
	// With hinges, the Y seems fine, so ignore this: wheelToBody = zofJointNewEx("body", zofPartEndPos(wheel,zofXyz(0,1,0)),zofXyzw(1,0,0,-0.5));
	wheelToBody = zofJointNew("body", zofPartEndPos(wheel,zofV3(0,1,0)));
	zofPartJointPut(wheel, wheelToBody);
	return wheel;
}

WheeledBase::WheeledBase() {
	zofPart casterBack, hips, support, wheelLeft;
	zofJoint
		casterBackToSupport, hipsToSupport, hipsToWheelLeft,
		supportToHips, supportToCasterBack;
	// Hips.
	hips = zofPartNewCapsule("hips", 0.12, 0.1);
	hipsToTorso = zofJointNew("torso", zofCapsuleEndPos(zofPartCapsule(hips),0.8));
	zofJointLimitsRotPut(hipsToTorso, zofV3(0,-0.5,0), zofV3(0,0.5,0));
	zofPartJointPut(hips, hipsToTorso);
	hipsToSupport = zofJointNew("support", zofCapsuleEndPos(zofPartCapsule(hips),-1));
	zofPartJointPut(hips, hipsToSupport);
	// Wheels.
	wheelLeft = humWheelNew();
	zofPartNamePut(wheelLeft, "wheelLeft");
	hipsToWheelLeft = zofJointNewEx(
		"wheelLeft",
		zofCapsuleEndPosEx(zofPartCapsule(hips), 1, zofV3(-1,0,0), -1),
		// With hinges, the Y seems fine, so ignore this: zofXyzw(0,1,0,0.5)
		zofV4(0,0,1,-0.5)
	);
	hipsToWheels.push_back(hipsToWheelLeft);
	// With hinges, the Y seems fine, so ignore this: zofJointLimitsRotPut(hipsToWheelLeft, zofXyz(0,0,zofNan()), zofXyz(0,0,zofNan()));
	zofJointLimitsRotPut(hipsToWheelLeft, zofV3(0,zofNan(),0), zofV3(0,zofNan(),0));
	zofPartJointPut(hips, hipsToWheelLeft);
	zofJointAttach(hipsToWheelLeft, zofPartJoint(wheelLeft, "body"));
	zofPartMirror(wheelLeft);
	hipsToWheels.push_back(zofPartJoint(hips, "wheelRight"));
	// Support.
	support = zofPartNewCylinder(
		"support",
		zofV3(0.8*zofCapsuleRadius(zofPartCapsule(hips)), 0.015, 0.15)
	);
	zofPartMaterialPut(support, zofMaterialNew(0xFF505050, 5));
	supportToHips = zofJointNew("hips", zofV3(0,0,0));
	zofPartJointPut(support, supportToHips);
	zofPartAttach(hips, support);
	// Casters.
	casterBack = zofPartNewCapsule(
		"casterBack",
		// Make the casters just big enough to reach the same base point as the wheels.
		zofPartRadii(wheelLeft).vals[0] - (zofPartPos(wheelLeft).vals[1] - zofPartPos(support).vals[1]) - zofPartRadii(support).vals[1],
		0
	);
	zofPartMaterialPut(casterBack, zofPartMaterial(wheelLeft));
	casterBackToSupport = zofJointNew("support", zofV3(0,0,0));
	zofPartJointPut(casterBack, casterBackToSupport);
	supportToCasterBack = zofJointNew("casterBack", zofPartEndPos(support,zofV3(0,-1,-1)));
	zofJointLimitsRotPut(supportToCasterBack, zofV3(zofNan(),zofNan(),zofNan()), zofV3(zofNan(),zofNan(),zofNan()));
	zofPartJointPut(support, supportToCasterBack);
	zofPartAttach(support, casterBack);
	zofPartCopyTo(casterBack, zofPartEndPos(support,zofV3(0,0,1)), "Back", "Front");
	// Base.
	zof = zofPartNewGroup("base", hips);
}

}
