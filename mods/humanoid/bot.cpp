#include "bot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace hum {

Arm::Arm(int side) {
	zofPart elbow, lower, shoulder, upper;
	zofJoint elbowToUpper, lowerToElbow, lowerToHand, upperToShoulder;
	// Shoulder.
	shoulder = zofPartNewCapsule("shoulder", 0.082, 0);
	shoulderToTorso = zofJointNew("torso", zofV3(0,0,0));
	zofJointLimitsRotPut(shoulderToTorso, zofV3(0, 0, -0.25), zofV3(0, 0, 1));
	zofPartJointPut(shoulder, shoulderToTorso);
	// Upper.
	upper = zofPartNewCapsule("upper", 0.05, 0.08);
	shoulderToUpper = zofJointNew("upper", zofV3(0,0,0));
	zofJointLimitsRotPut(shoulderToUpper, zofV3(-0.5,0,0), zofV3(1,0,0));
	zofPartJointPut(shoulder, shoulderToUpper);
	upperToShoulder = zofJointNew("shoulder", zofCapsuleEndPos(zofPartCapsule(upper), 0.5));
	zofPartJointPut(upper, upperToShoulder);
	zofPartAttach(shoulder, upper);
	// Elbow. Needed because multiple DOFs without it are causing trouble.
	elbow = zofPartNewCapsule("elbow", 0.05, 0.02);
	upperToElbow = zofJointNew("elbow", zofCapsuleEndPos(zofPartCapsule(upper), -1));
	zofJointLimitsRotPut(upperToElbow, zofV3(0,-0.5,0), zofV3(0,0.5,0));
	zofPartJointPut(upper, upperToElbow);
	elbowToUpper = zofJointNew("upper", zofCapsuleEndPos(zofPartCapsule(elbow), 0.2));
	zofPartJointPut(elbow, elbowToUpper);
	zofPartAttachSwap(upper, elbow, side < 0 ? zofFalse : zofTrue);
	// Lower.
	lower = zofPartNewCapsule("lower", 0.05, 0.08);
	elbowToLower = zofJointNew("lower", zofCapsuleEndPos(zofPartCapsule(elbow), -0.5));
	zofJointLimitsRotPut(elbowToLower, zofV3(0,0,0), zofV3(0.8,0,0));
	zofPartJointPut(elbow, elbowToLower);
	lowerToElbow = zofJointNew("elbow", zofCapsuleEndPos(zofPartCapsule(lower), 0.5));
	zofPartJointPut(lower, lowerToElbow);
	zofPartAttach(elbow, lower);
	// Hand joints.
	lowerToHand = zofJointNew(side < 0 ? "handLeft" : "handRight", zofCapsuleEndPos(zofPartCapsule(lower), -1));
	zofJointLimitsRotPut(lowerToHand, zofV3(0,-0.5,0), zofV3(0,0.7,0));
	zofPartJointPut(lower, lowerToHand);
	// Arm.
	zof = zofPartNewGroup(side < 0 ? "armLeft" : "armRight", shoulder);
	// Actually attach hand last since we want it separate from the arm group.
	attach(hand = new Hand(side), side > 0);
}

Finger::Finger(int side, int phalanxCount) {
	zofPart current, spread;
	// Spread.
	// TODO Could 6-DOF avoid this? Or do I still fear 6-DOF instability?
	spread = zofPartNewCapsule("spread", 0.015, 0);
	spreadToHand = zofJointNew("hand", zofCapsuleEndPos(zofPartCapsule(spread), 0.5));
	zofJointLimitsRotPut(spreadToHand, zofV3(-0.2,0,0), zofV3(0.2,0,0));
	zofPartJointPut(spread, spreadToHand);
	current = spread;
	for (int p = 0; p < phalanxCount; p++) {
		zofPart next;
		zofJoint currentToNext, nextToCurrent;
		// TODO Number them?
		next = zofPartNewCapsule("phalanx", 0.015, 0.01);
		currentToNext = zofJointNew("next", zofCapsuleEndPos(zofPartCapsule(current), -0.5));
		curls.push_back(currentToNext);
		zofJointLimitsRotPut(currentToNext, zofV3(0,0,0), zofV3(0,0,0.5));
		zofPartJointPut(current, currentToNext);
		nextToCurrent = zofJointNew("prev", zofCapsuleEndPos(zofPartCapsule(next), 0.5));
		zofPartJointPut(next, nextToCurrent);
		zofJointAttachSwap(currentToNext, nextToCurrent, side < 0 ? zofTrue : zofFalse);
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
	wrist = zofPartNewCapsule("wrist", 0.05, 0.01);
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
	thumbTwist = zofPartNewCapsule("thumbTwist", 0.022, 0.007);
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
	skull = zofPartNewCapsule("skull", 0.095, 0.023);
	//zofPartMaterialPut(skull, zofMaterialNew(0xFFA0A0A0, 0.1));
	skullToNeck = zofJointNewEx(
		"neck",
		zofCapsuleEndPos(zofPartCapsule(skull), -1),
		// TODO Rotate around Z for Y sideways? Or just make X the axis of rotation?
		zofV4(0,1,0,0)
	);
	zofPartJointPut(skull, skullToNeck);
	// Neck.
	neck = zofPartNewCapsule("neck", 0.07, 0);
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
	eyeLeft = zofPartNewCapsule("eyeLeft", 0.022, 0);
	zofPartMaterialPut(eyeLeft, zofMaterialNew(0xFF0060A0,0.001));
	eyeLeftToSkull = zofJointNew("skull", zofV3(0,0,0));
	zofPartJointPut(eyeLeft, eyeLeftToSkull);
	skullToEyeLeft = zofJointNew(
		"eyeLeft",
		zofCapsuleEndPosEx(zofPartCapsule(skull), 0.8, zofV3(-0.25,0,1), 1)
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
	zofPart chest = zofPartNewCapsule("chest", 0.18, 0.1);
	chestToAbdomen = zofJointNew("abdomen", zofCapsuleEndPos(zofPartCapsule(chest),-0.5));
	zofPartJointPut(chest, chestToAbdomen);
	chestToHead = zofJointNew("head", zofCapsuleEndPos(zofPartCapsule(chest), 1));
	zofPartJointPut(chest, chestToHead);
	// Abdomen.
	zofPart abdomen = zofPartNewCapsule("abdomen", 0.14, 0.08);
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

WheeledBase::WheeledBase() {
	zofPart casterBack, hips, support, wheel;
	zofJoint casterBackToSupport, hipsToSupport, supportToHips, supportToCasterBack;
	// Hips.
	hips = zofPartNewCapsule("hips", 0.19, 0.14);
	hipsToTorso = zofJointNew("torso", zofCapsuleEndPos(zofPartCapsule(hips),0.8));
	zofJointLimitsRotPut(hipsToTorso, zofV3(0,-0.5,0), zofV3(0,0.5,0));
	zofPartJointPut(hips, hipsToTorso);
	hipsToSupport = zofJointNew("support", zofCapsuleEndPos(zofPartCapsule(hips),-1));
	zofPartJointPut(hips, hipsToSupport);
	// Wheels.
	for (int side = -1; side <= 1; side += 2) {
		wheel = makeWheel(side);
		zofJoint hipsToWheel = zofJointNewEx(
			side < 0 ? "wheelLeft" : "wheelRight",
			zofCapsuleEndPosEx(zofPartCapsule(hips), 1, zofV3(side,0,0), -1),
			// With hinges, the Y seems fine, so ignore this: zofXyzw(0,1,0,0.5)
			zofV4(0,0,1,-0.5)
		);
		hipsToWheels.push_back(hipsToWheel);
		// With hinges, the Y seems fine, so ignore this: zofJointLimitsRotPut(hipsToWheelLeft, zofXyz(0,0,zofNan()), zofXyz(0,0,zofNan()));
		zofJointLimitsRotPut(hipsToWheel, zofV3(0,zofNan(),0), zofV3(0,zofNan(),0));
		zofPartJointPut(hips, hipsToWheel);
		zofPartAttachSwap(hips, wheel, zofTrue);
		// TODO Someday: zofPartMirror(wheelLeft);
	}
	// Support.
	support = zofPartNewCylinder(
		"support",
		zofV3(0.8*zofCapsuleRadius(zofPartCapsule(hips)), 0.015, 1.2*zofCapsuleRadius(zofPartCapsule(hips)))
	);
	zofPartMaterialPut(support, zofMaterialNew(0xFF505050, 5));
	supportToHips = zofJointNew("hips", zofV3(0,0,0));
	zofPartJointPut(support, supportToHips);
	zofPartAttach(hips, support);
	// Casters.
	casterBack = zofPartNewCapsule(
		"casterBack",
		// Make the casters just big enough to reach the same base point as the wheels.
		zofPartRadii(wheel).vals[0] - (zofPartPos(wheel).vals[1] - zofPartPos(support).vals[1]) - zofPartRadii(support).vals[1],
		0
	);
	zofPartMaterialPut(casterBack, zofPartMaterial(wheel));
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

zofPart WheeledBase::makeWheel(int side) {
	zofPart wheel;
	zofJoint wheelToBody;
	wheel = zofPartNewCylinder(side < 0 ? "wheelLeft" : "wheelRight", zofV3(0.25,0.04,0.25));
	zofPartMaterialPut(wheel, zofMaterialNew(0xFF202020, 20));
	// With hinges, the Y seems fine, so ignore this: wheelToBody = zofJointNewEx("body", zofPartEndPos(wheel,zofXyz(0,1,0)),zofXyzw(1,0,0,-0.5));
	wheelToBody = zofJointNew("hips", zofPartEndPos(wheel,zofV3(0,-side,0)));
	zofPartJointPut(wheel, wheelToBody);
	return wheel;
}

}
