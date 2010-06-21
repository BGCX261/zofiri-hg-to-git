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
	// TODO Build head.
	zof_part skull = zof_part_new_capsule("skull", 0.06, 0.01);
	return zof_part_new_group("head", skull);
}

zof_part humanoid_new(void) {
	zof_part torso = torso_new();
	zof_part_attach(torso, head_new());
	return zof_part_new_group("humanoid", torso);
}

zof_part torso_new(void) {
	// TODO Once we have material fill, save material assignment for the end.
	// TODO Or support find & replace for materials? Or "styles" like CSS?
	// TODO Or should attaching a default material to custom assign the custom?
	zof_material metal = zof_material_new(0xFF808080,1);
	// Chest.
	zof_part chest = zof_part_new_capsule("chest", 0.1, 0.0725);
	zof_part_material_put(chest, metal);
	zof_joint joint_to_abdomen = zof_joint_new(
		"abdomen",
		zof_capsule_end_pos(zof_part_capsule(chest), -0.5),
		zof_xyzw(0,1,0,0)
	);
	zof_part_joint_put(chest, joint_to_abdomen);
	// Abdomen.
	zof_part abdomen = zof_part_new_capsule("abdomen", 0.08, 0.05);
	zof_part_material_put(abdomen, metal);
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
