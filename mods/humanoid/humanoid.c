#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zof.h"

zofPart humArmNew(zofInt side);

zofPart humBaseWheeledNew(void);

zofPart humFingerNew(zofInt side, zofUint phalanxCount);

zofPart humHandNew(zofInt side);

zofPart humHeadNew(void);

zofPart humHumanoidNew(void);

zofPart humTorsoNew(void);

void humUpdate(zofSim sim, zofAny data);

/**
 * So far, our tires are simple, but it's so easy to imagine them as
 * independent parts, and they could have caps or whatnot, too.
 *
 * TODO Introduce size parameters and so on.
 */
zofPart humWheelNew(void);

zofModExport zofBool zofSimInit(zofMod mod, zofSim sim) {
	// TODO Obviously need more to the humanoid than this.
	// TODO Especially want group parts.
	zofPart humanoid = humHumanoidNew();
	zofPartPosPut(humanoid, zofXyz(-0.2,-zofPartExtents(humanoid).min.vals[1],0.2));
	zofSimPartAdd(sim, humanoid);
	zofSimUpdaterAdd(sim, humUpdate, humanoid);
	return zofTrue;
}

zofPart humArmNew(zofInt side) {
	zofPart arm, elbow, lower, shoulder, upper;
	zofJoint
		elbowToLower, elbowToUpper, lowerToElbow, lowerToHand,
		shoulderToUpper, shoulderToTorso, upperToElbow, upperToShoulder;
	// Shoulder.
	shoulder = zofPartNewCapsule("shoulder", 0.05, 0);
	shoulderToTorso = zofJointNew("torso", zofXyz(0,0,0));
	zofJointLimitsRotPut(
		shoulderToTorso,
		zofXyz(0, 0, -zofPi * (side < 0 ? 0.25 : 1)),
		zofXyz(0, 0, zofPi * (side < 0 ? 1 : 0.25))
	);
	zofPartJointPut(shoulder, shoulderToTorso);
	// Upper.
	upper = zofPartNewCapsule("upper", 0.03, 0.05);
	shoulderToUpper = zofJointNew("upper", zofXyz(0,0,0));
	zofJointLimitsRotPut(shoulderToUpper, zofXyz(-zofPi/2,0,0), zofXyz(zofPi,0,0));
	zofPartJointPut(shoulder, shoulderToUpper);
	upperToShoulder = zofJointNew("shoulder", zofCapsuleEndPos(zofPartCapsule(upper), 0.5));
	zofPartJointPut(upper, upperToShoulder);
	zofPartAttach(shoulder, upper);
	// Elbow. Needed because multiple DOFs without it are causing trouble.
	elbow = zofPartNewCapsule("elbow", 0.03, 0.01);
	upperToElbow = zofJointNew("elbow", zofCapsuleEndPos(zofPartCapsule(upper), -1));
	zofJointLimitsRotPut(upperToElbow, zofXyz(0,-zofPi/2,0), zofXyz(0,zofPi/2,0));
	zofPartJointPut(upper, upperToElbow);
	elbowToUpper = zofJointNew("upper", zofCapsuleEndPos(zofPartCapsule(elbow), 0.2));
	zofPartJointPut(elbow, elbowToUpper);
	zofPartAttach(upper, elbow);
	// Lower.
	lower = zofPartNewCapsule("lower", 0.03, 0.05);
	elbowToLower = zofJointNew("lower", zofCapsuleEndPos(zofPartCapsule(elbow), -0.5));
	zofJointLimitsRotPut(elbowToLower, zofXyz(0,0,0), zofXyz(0.8*zofPi,0,0));
	zofPartJointPut(elbow, elbowToLower);
	lowerToElbow = zofJointNew("elbow", zofCapsuleEndPos(zofPartCapsule(lower), 0.5));
	zofPartJointPut(lower, lowerToElbow);
	zofPartAttach(elbow, lower);
	// Hand.
	lowerToHand = zofJointNew(side < 0 ? "handLeft" : "handRight", zofCapsuleEndPos(zofPartCapsule(lower), -1));
	zofJointLimitsRotPut(lowerToHand, zofXyz(0,-zofPi/2,0), zofXyz(0,0.7*zofPi,0));
	zofPartJointPut(lower, lowerToHand);
	// Arm.
	arm = zofPartNewGroup(side < 0 ? "armLeft" : "armRight", shoulder);
	zofPartAttach(arm, humHandNew(side));
	return arm;
}

zofPart humBaseWheeledNew(void) {
	zofPart casterBack, hips, support, wheelLeft;
	zofJoint
		casterBackToSupport, hipsToSupport, hipsToTorso, hipsToWheelLeft,
		supportToHips, supportToCasterBack;
	// Hips.
	hips = zofPartNewCapsule("hips", 0.12, 0.1);
	hipsToTorso = zofJointNew("torso", zofCapsuleEndPos(zofPartCapsule(hips),0.8));
	zofPartJointPut(hips, hipsToTorso);
	hipsToSupport = zofJointNew("support", zofCapsuleEndPos(zofPartCapsule(hips),-1));
	zofPartJointPut(hips, hipsToSupport);
	// Wheels.
	wheelLeft = humWheelNew();
	zofPartNamePut(wheelLeft, "wheelLeft");
	hipsToWheelLeft = zofJointNewEx(
		"wheelLeft",
		zofCapsuleEndPosEx(zofPartCapsule(hips), 1, zofXyz(-1,0,0), -1),
		// With hinges, the Y seems fine, so ignore this: zofXyzw(0,1,0,zofPi/2)
		zofXyzw(0,0,1,-zofPi/2)
	);
	// With hinges, the Y seems fine, so ignore this: zofJointLimitsRotPut(hipsToWheelLeft, zofXyz(0,0,zofNan()), zofXyz(0,0,zofNan()));
	zofJointLimitsRotPut(hipsToWheelLeft, zofXyz(0,zofNan(),0), zofXyz(0,zofNan(),0));
	zofPartJointPut(hips, hipsToWheelLeft);
	zofJointAttach(hipsToWheelLeft, zofPartJoint(wheelLeft, "body"));
	zofPartMirror(wheelLeft);
	// Support.
	support = zofPartNewCylinder(
		"support",
		zofXyz(0.8*zofCapsuleRadius(zofPartCapsule(hips)), 0.015, 0.15)
	);
	zofPartMaterialPut(support, zofMaterialNew(0xFF505050, 5));
	supportToHips = zofJointNew("hips", zofXyz(0,0,0));
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
	casterBackToSupport = zofJointNew("support", zofXyz(0,0,0));
	zofPartJointPut(casterBack, casterBackToSupport);
	supportToCasterBack = zofJointNew("casterBack", zofPartEndPos(support,zofXyz(0,-1,-1)));
	zofJointLimitsRotPut(supportToCasterBack, zofXyz(zofNan(),zofNan(),zofNan()), zofXyz(zofNan(),zofNan(),zofNan()));
	zofPartJointPut(support, supportToCasterBack);
	zofPartAttach(support, casterBack);
	zofPartCopyTo(casterBack, zofPartEndPos(support,zofXyz(0,0,1)), "Back", "Front");
	// Base.
	return zofPartNewGroup("base", hips);
}

zofPart humFingerNew(zofInt side, zofUint phalanxCount) {
	zofInt p;
	zofPart current, spread;
	zofJoint fingerToHand;
	// Spread.
	// TODO Could 6-DOF avoid this? Or do I still fear 6-DOF instability?
	spread = zofPartNewCapsule("spread", 0.01, 0);
	fingerToHand = zofJointNew("hand", zofCapsuleEndPos(zofPartCapsule(spread), 0.5));
	zofJointLimitsRotPut(fingerToHand, zofXyz(-zofPi/5,0,0), zofXyz(zofPi/5,0,0));
	zofPartJointPut(spread, fingerToHand);
	current = spread;
	for (p = 0; p < phalanxCount; p++) {
		zofPart next;
		zofJoint currentToNext, nextToCurrent;
		// TODO Number them?
		next = zofPartNewCapsule("phalanx", 0.01, 0.006);
		currentToNext = zofJointNew("next", zofCapsuleEndPos(zofPartCapsule(current), -0.5));
		zofJointLimitsRotPut(currentToNext, zofXyz(0, 0, side < 0 ? -zofPi/2 : 0), zofXyz(0, 0, side < 0 ? 0 : zofPi/2));
		zofPartJointPut(current, currentToNext);
		nextToCurrent = zofJointNew("prev", zofCapsuleEndPos(zofPartCapsule(next), 0.5));
		zofPartJointPut(next, nextToCurrent);
		zofJointAttach(currentToNext, nextToCurrent);
		current = next;
	}
	return zofPartNewGroup("fingerLeft", spread);
}

zofPart humHandNew(zofInt side) {
	zofInt f;
	zofString fingerJointName;
	zofPart thumbTwist, wrist;
	zofJoint thumbTwistToThumb, thumbTwistToWrist, wristToArm, wristToThumbTwist;
	// Wrist.
	wrist = zofPartNewCapsule("wrist", 0.03, 0.005);
	wristToArm = zofJointNew(side < 0 ? "armLeft" : "armRight", zofCapsuleEndPos(zofPartCapsule(wrist), 0.3));
	zofPartJointPut(wrist, wristToArm);
	// Fingers.
	// TODO Better strings yet???
	fingerJointName = zofStringNewCopy("finger##");
	for (f = 0; f < 3; f++) {
		zofPart finger;
		zofJoint wristToFinger;
		finger = humFingerNew(side, 3);
		sprintf(fingerJointName, "finger%d", f);
		wristToFinger = zofJointNew(fingerJointName, zofCapsuleEndPosEx(zofPartCapsule(wrist), 1, zofXyz(-side,-2,2*(f-1)), 1));
		zofPartJointPut(wrist, wristToFinger);
		zofJointAttach(wristToFinger, zofPartJoint(finger, "hand"));
	}
	// TODO zofFree?
	free(fingerJointName);
	// Thumb.
	// TODO How does side affect this mess?
	thumbTwist = zofPartNewCapsule("thumbTwist", 0.014, 0.004);
	wristToThumbTwist = zofJointNew("thumbTwist", zofCapsuleEndPosEx(zofPartCapsule(wrist), 1, zofXyz(-side*1.5,0.7,1), 0));
	zofJointLimitsRotPut(wristToThumbTwist, zofXyz(0,-zofPi/4,0), zofXyz(0,zofPi/2,0));
	zofPartJointPut(wrist, wristToThumbTwist);
	thumbTwistToWrist = zofJointNewEx("wrist", zofCapsuleEndPos(zofPartCapsule(thumbTwist),0.2), zofXyzw(0,0,1,-zofPi/2));
	zofPartJointPut(thumbTwist, thumbTwistToWrist);
	thumbTwistToThumb = zofJointNewEx(
		"thumb",
		zofCapsuleEndPosEx(zofPartCapsule(thumbTwist), -1, zofXyz(-1,2,0), -1),
		zofXyzw(0, 1, 0, 0.75*zofPi)
	);
	zofPartJointPut(thumbTwist, thumbTwistToThumb);
	zofJointAttach(thumbTwistToThumb, zofPartJoint(humFingerNew(side, 2), "hand"));
	zofPartAttach(wrist, thumbTwist);
	// Hand.
	return zofPartNewGroup(side < 0 ? "handLeft" : "handRight", wrist);
}

zofPart humHeadNew(void) {
	zofPart eyeLeft, neck, skull;
	zofJoint eyeLeftToSkull, neckToSkull, neckToTorso, skullToEyeLeft, skullToNeck;
	// Skull.
	skull = zofPartNewCapsule("skull", 0.06, 0.01);
	//zofPartMaterialPut(skull, zofMaterialNew(0xFFA0A0A0, 0.1));
	skullToNeck = zofJointNewEx(
		"neck",
		zofCapsuleEndPos(zofPartCapsule(skull), -1),
		// TODO Rotate around Z for Y sideways? Or just make X the axis of rotation?
		zofXyzw(0,1,0,0)
	);
	zofPartJointPut(skull, skullToNeck);
	// Neck.
	neck = zofPartNewCapsule("neck", 0.04, 0);
	neckToTorso = zofJointNew("torso", zofCapsuleEndPos(zofPartCapsule(neck),-0.3));
	zofPartJointPut(neck, neckToTorso);
	neckToSkull = zofJointNewEx(
		"skull",
		zofCapsuleEndPos(zofPartCapsule(neck), 0.3),
		// TODO Rotate around Z for Y sideways? Or just make X the axis of rotation?
		zofXyzw(0,1,0,0)
	);
	zofJointLimitsRotPut(neckToSkull, zofXyz(-zofPi/2,0,0), zofXyz(zofPi/2,0,0));
	zofPartJointPut(neck, neckToSkull);
	zofPartAttach(skull, neck);
	// Eyes.
	// TODO Consider making eyes a separately defined part.
	eyeLeft = zofPartNewCapsule("eyeLeft", 0.015, 0);
	zofPartMaterialPut(eyeLeft, zofMaterialNew(0xFF0060A0,0.001));
	eyeLeftToSkull = zofJointNew("skull", zofXyz(0,0,0));
	zofPartJointPut(eyeLeft, eyeLeftToSkull);
	skullToEyeLeft = zofJointNew(
		"eyeLeft",
		zofCapsuleEndPosEx(zofPartCapsule(skull), 0.8, zofXyz(-0.2,0,1), 1)
	);
	zofPartJointPut(skull, skullToEyeLeft);
	zofPartAttach(skull, eyeLeft);
	zofPartMirror(eyeLeft);
	// Head.
	return zofPartNewGroup("head", skull);
}

zofPart humHumanoidNew(void) {
	zofPart humanoid, torso;
	// Torso, head, and base.
	torso = humTorsoNew();
	zofPartAttach(torso, humHeadNew());
	zofPartAttach(torso, humBaseWheeledNew());
	// Arms.
	zofPartAttach(torso, humArmNew(-1));
	zofPartAttach(torso, humArmNew(1));
	// TODO zofPartMirror(armLeft);
	// Humanoid.
	humanoid = zofPartNewGroup("humanoid", torso);
	zofPartMaterialPut(humanoid, zofMaterialNew(0xFF808080,1));
	return humanoid;
}

zofPart humTorsoNew(void) {
	zofPart abdomen, chest;
	zofJoint abdomenToBase, abdomenToChest, chestToAbdomen, chestToArmLeft, chestToHead;
	// Chest.
	chest = zofPartNewCapsule("chest", 0.1, 0.0725);
	chestToAbdomen = zofJointNew("abdomen", zofCapsuleEndPos(zofPartCapsule(chest),-0.5));
	zofPartJointPut(chest, chestToAbdomen);
	chestToHead = zofJointNew("head", zofCapsuleEndPos(zofPartCapsule(chest), 1));
	zofPartJointPut(chest, chestToHead);
	// Abdomen.
	abdomen = zofPartNewCapsule("abdomen", 0.08, 0.05);
	abdomenToChest = zofJointNew("chest", zofCapsuleEndPos(zofPartCapsule(abdomen),0.5));
	zofPartJointPut(abdomen, abdomenToChest);
	abdomenToBase = zofJointNew("base", zofCapsuleEndPos(zofPartCapsule(abdomen),-0.5));
	zofPartJointPut(abdomen, abdomenToBase);
	zofPartAttach(chest, abdomen);
	// Arm joint.
	// TODO It would make sense to mirror it here and then attach a mirrored arm.
	// TODO However, that's less convenient at the moment.
	// TODO It might even make sense to make shoulders part of the torso.
	chestToArmLeft = zofJointNew("armLeft", zofCapsuleEndPosEx(zofPartCapsule(chest), 1.4, zofXyz(-1.5,1,0), 1));
	zofPartJointPut(chest, chestToArmLeft);
	zofPartJointPut(chest, zofJointNew("armRight", zofCapsuleEndPosEx(zofPartCapsule(chest), 1.4, zofXyz(1.5,1,0), 1)));
	// Torso.
	return zofPartNewGroup("torso", chest);
}

void humUpdate(zofSim sim, zofAny data) {
	static zofInt i = 0;
	static const zofInt max = 400;
	zofPart humanoid = (zofPart)data;
	i = (i + 1) % max;
	//zofJointVelPut(zofPartJoint(humanoid, "//neck/skull"), 0.0035);
	zofJointPosPut(zofPartJoint(humanoid, "//neck/skull"), 0);
	zofJointPosPut(zofPartJoint(humanoid, "//chest/armLeft"), 0.25 * zofPi);
	zofJointPosPut(zofPartJoint(humanoid, "//chest/armRight"), -0.25 * zofPi);
	zofJointPosPut(zofPartJoint(humanoid, "//shoulder/upper"), 0 * zofPi);
	zofJointPosPut(zofPartJoint(humanoid, "//upper/elbow"), i < max/2 ? zofPi/2 : -zofPi/2);
	//zofJointPosPut(zofPartJoint(humanoid, "//upper/elbow"), -0.25 * zofPi);
	//zofJointPosPut(zofPartJoint(humanoid, "//elbow/lower"), 0.25 * zofPi);
	//zofJointPosPut(zofPartJoint(humanoid, "//lower/handLeft"), 0.25 * zofPi);
	//zofJointPosPut(zofPartJoint(humanoid, "//wrist/thumbTwist"), i < max/2 ? zofPi : -zofPi);
	//zofJointPosPut(zofPartJoint(humanoid, "//wrist/thumbTwist"), 0);
	zofJointVelPut(zofPartJoint(humanoid, "//hips/wheelLeft"), 0.5);
	zofJointVelPut(zofPartJoint(humanoid, "//hips/wheelRight"), 0.5);
	//zofJointPosPut(zofPartJoint(humanoid, "//hips/wheelLeft"), 0);
	//zofJointPosPut(zofPartJoint(humanoid, "//hips/wheelRight"), 0);
}

zofPart humWheelNew(void) {
	zofPart wheel;
	zofJoint wheelToBody;
	wheel = zofPartNewCylinder("wheel", zofXyz(0.18,0.03,0.18));
	zofPartMaterialPut(wheel, zofMaterialNew(0xFF202020, 20));
	// With hinges, the Y seems fine, so ignore this: wheelToBody = zofJointNewEx("body", zofPartEndPos(wheel,zofXyz(0,1,0)),zofXyzw(1,0,0,-zofPi/2));
	wheelToBody = zofJointNew("body", zofPartEndPos(wheel,zofXyz(0,1,0)));
	zofPartJointPut(wheel, wheelToBody);
	return wheel;
}
