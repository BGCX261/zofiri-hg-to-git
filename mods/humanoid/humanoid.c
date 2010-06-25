#include <stdio.h>
#include "zof.h"

zof_part arm_new(int side_x);
zof_part base_new(void);
zof_part hand_new(int side_x);
zof_part head_new(void);
zof_part humanoid_new(void);
zof_part torso_new(void);

zof_bool sim_init(zof_mod mod, zof_sim sim) {
	// TODO Obviously need more to the humanoid than this.
	// TODO Especially want group parts.
	zof_part humanoid = humanoid_new();
	zof_part_pos_put(humanoid, zof_xyz(-0.2,1,0.2));
	zof_sim_part_add(sim, humanoid);
	return zof_true;
}

zof_part head_new(void) {
	// Neck.
    zof_part neck = zof_part_new_capsule("neck", 0.04, 0);
	zof_joint joint_to_torso = zof_joint_new(
		"torso",
		zof_capsule_end_pos(zof_part_capsule(neck), -0.3),
		zof_xyzw(0,1,0,0)
	);
	zof_part_joint_put(neck, joint_to_torso);
	zof_joint joint_to_skull = zof_joint_new(
		"skull",
		zof_capsule_end_pos(zof_part_capsule(neck), 0.3),
		// TODO Rotate around Z for Y sideways? Or just make X the axis of rotation?
		zof_xyzw(0,1,0,0)
	);
	zof_part_joint_put(neck, joint_to_skull);
	// Skull.
	zof_part skull = zof_part_new_capsule("skull", 0.06, 0.01);
	zof_joint joint_to_neck = zof_joint_new(
		"neck",
		zof_capsule_end_pos(zof_part_capsule(skull), -1),
		// TODO Rotate around Z for Y sideways? Or just make X the axis of rotation?
		zof_xyzw(0,1,0,0)
	);
	zof_part_joint_put(skull, joint_to_neck);
	// Attach and return group.
	zof_part_attach(skull, neck);
	//printf("skull kind: %d\n", zof_part_part_kind(skull));
	return zof_part_new_group("head", skull);
}

zof_part humanoid_new(void) {
	zof_part torso = torso_new();
	//printf("torso kind: %d\n", zof_part_part_kind(torso));
	zof_part_attach(torso, head_new());
	zof_part humanoid = zof_part_new_group("humanoid", torso);
	zof_part_material_put(humanoid, zof_material_new(0xFF808080,1));
	return humanoid;
}

zof_part torso_new(void) {
	// Chest.
	zof_part chest = zof_part_new_capsule("chest", 0.1, 0.0725);
	zof_joint joint_to_abdomen = zof_joint_new(
		"abdomen",
		zof_capsule_end_pos(zof_part_capsule(chest), -0.5),
		zof_xyzw(0,1,0,0)
	);
	zof_part_joint_put(chest, joint_to_abdomen);
	zof_joint joint_to_head = zof_joint_new(
		"head",
		zof_capsule_end_pos(zof_part_capsule(chest), 1),
		zof_xyzw(0,1,0,0)
	);
	zof_part_joint_put(chest, joint_to_head);
	// Abdomen.
	zof_part abdomen = zof_part_new_capsule("abdomen", 0.08, 0.05);
	zof_joint joint_to_chest = zof_joint_new(
		"chest",
		zof_capsule_end_pos(zof_part_capsule(abdomen), 0.5),
		zof_xyzw(0,1,0,0)
	);
	zof_part_joint_put(abdomen, joint_to_chest);
	// Attach them.
	zof_part_attach(chest, abdomen);
	return zof_part_new_group("torso", chest);
}
