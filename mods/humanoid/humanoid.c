#include <stdio.h>
#include "zof.h"

zofPart humArmLeftNew();

zofPart humBaseWheeledNew(void);

zofPart humHandLeftNew();

zofPart humHeadNew(void);

zofPart humHumanoidNew(void);

void humUpdate(zofSim sim, zofAny data);

zofPart humTorsoNew(void);

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
    	zofXyzw(0,1,0,zofPi/2)
    	//zofXyzw(0,0,1,-zofPi/2)
    );
    zofJointLimitsRotPut(hipsToWheelLeft, zofXyz(0,0,zofNan), zofXyz(0,0,zofNan));
    //zofJointLimitsRotPut(hipsToWheelLeft, zofXyz(0,zofNan,0), zofXyz(0,zofNan,0));
    zofPartJointPut(hips, hipsToWheelLeft);
    zofJointAttach(hipsToWheelLeft, zofPartJoint(wheelLeft, "body"));
    zofPartMirror(wheelLeft);
    // Support.
    support = zofPartNewCylinder(
		"support",
		zofXyz(0.7*zofCapsuleRadius(zofPartCapsule(hips)), 0.02, 0.15)
    );
    zofPartMaterialPut(support, zofMaterialNew(0xFF505050, 5));
    supportToHips = zofJointNew("hips", zofXyz(0,0,0));
    zofPartJointPut(support, supportToHips);
    zofPartAttach(hips, support);
    // Casters.
    casterBack = zofPartNewCapsule(
    	"casterBack",
    	// Make the casters just big enough to reach the same base point as the wheels.
    	zofPartRadii(wheelLeft).vals[0] - (zofPartPos(wheelLeft).vals[1] - zofPartPos(support).vals[1]),
    	0
    );
    zofPartMaterialPut(casterBack, zofPartMaterial(wheelLeft));
    casterBackToSupport = zofJointNew("support", zofXyz(0,0,0));
    zofPartJointPut(casterBack, casterBackToSupport);
    supportToCasterBack = zofJointNew("casterBack", zofPartEndPos(support,zofXyz(0,0,-1)));
    zofJointLimitsRotPut(supportToCasterBack, zofXyz(zofNan,zofNan,zofNan), zofXyz(zofNan,zofNan,zofNan));
    zofPartJointPut(support, supportToCasterBack);
    zofPartAttach(support, casterBack);
    zofPartCopyTo(casterBack, zofPartEndPos(support,zofXyz(0,0,1)), "Back", "Front");
    // Base.
    return zofPartNewGroup("base", hips);
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
	torso = humTorsoNew();
	zofPartAttach(torso, humHeadNew());
	zofPartAttach(torso, humBaseWheeledNew());
	humanoid = zofPartNewGroup("humanoid", torso);
	//humanoid = humBaseWheeledNew();
	zofPartMaterialPut(humanoid, zofMaterialNew(0xFF808080,1));
	return humanoid;
}

void humUpdate(zofSim sim, zofAny data) {
	static zofNum dir = 1;
	static zofNum vel = 0;
	zofPart humanoid;
	zofJoint neckToSkull;
	humanoid = (zofPart)data;
	neckToSkull = zofPartJoint(humanoid, "//neck/skull");
	if (vel < -5 || 5 < vel) {
		dir = -dir;
		//fprintf(stderr, "Changing dir for %s!\n");
	}
	vel += dir * 0.1;
	zofJointVelPut(neckToSkull, vel > 0 ? 5 : -5);
	zofJointVelPut(zofPartJoint(humanoid, "//hips/wheelLeft"), -1.8);
	zofJointVelPut(zofPartJoint(humanoid, "//hips/wheelRight"), 1);
}

zofPart humTorsoNew(void) {
	zofPart abdomen, chest;
	zofJoint abdomenToBase, abdomenToChest, chestToAbdomen, chestToHead;
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
	// Torso.
	return zofPartNewGroup("torso", chest);
}

zofPart humWheelNew(void) {
	zofPart wheel;
	zofJoint wheelToBody;
	wheel = zofPartNewCylinder("wheel", zofXyz(0.18,0.04,0.18));
	zofPartMaterialPut(wheel, zofMaterialNew(0xFF202020, 20));
	wheelToBody = zofJointNewEx("body", zofPartEndPos(wheel,zofXyz(0,1,0)),zofXyzw(1,0,0,-zofPi/2));
	//wheelToBody = zofJointNew("body", zofPartEndPos(wheel,zofXyz(0,1,0)));
	zofPartJointPut(wheel, wheelToBody);
	return wheel;
}
