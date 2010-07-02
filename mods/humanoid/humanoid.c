#include <stdio.h>
#include "zof.h"

zof_part arm_left_new();

zof_part base_wheeled_new(void);

zof_part hand_left_new();

zof_part head_new(void);

zof_part humanoid_new(void);

/**
 * So far, our tires are simple, but it's so easy to imagine them as
 * independent parts, and they could have caps or whatnot, too.
 *
 * TODO Introduce size parameters and so on.
 */
zof_part wheel_new(void);

zof_part torso_new(void);

zof_mod_export zof_bool sim_init(zof_mod mod, zof_sim sim) {
	// TODO Obviously need more to the humanoid than this.
	// TODO Especially want group parts.
	zof_part humanoid = humanoid_new();
	zof_part_pos_put(humanoid, zof_xyz(-0.2,1,0.2));
	zof_sim_part_add(sim, humanoid);
	return zof_true;
}

zof_part base_wheeled_new(void) {
	zof_part hips, support, wheel_left;
	zof_joint hips_to_support, hips_to_torso, support_to_hips, hips_to_wheel_left;
	// Hips.
    hips = zof_part_new_capsule("hips", 0.12, 0.1);
    hips_to_torso = zof_joint_new(
		"torso",
		zof_capsule_end_pos(zof_part_capsule(hips), 0.8),
		zof_xyzw(0,1,0,0)
    );
    zof_part_joint_put(hips, hips_to_torso);
    hips_to_support = zof_joint_new(
		"support",
		zof_capsule_end_pos(zof_part_capsule(hips), -1),
		zof_xyzw(0,1,0,0)
    );
    zof_part_joint_put(hips, hips_to_support);
    // Wheels.
    wheel_left = wheel_new();
    zof_part_name_put(wheel_left, "wheel_left");
    hips_to_wheel_left = zof_joint_new(
    	"wheel_left",
    	zof_capsule_end_pos_ex(zof_part_capsule(hips), 1, zof_xyz(-1,0,0), -1),
    	zof_xyzw(0,0,1,-zof_pi/2)
    );
    zof_part_joint_put(hips, hips_to_wheel_left);
    zof_joint_attach(hips_to_wheel_left, zof_part_joint(wheel_left, "body"));
    zof_part_mirror(wheel_left);
    // Support.
    support = zof_part_new_cylinder(
		"support",
		zof_xyz(0.9*zof_capsule_radius(zof_part_capsule(hips)), 0.02, 0.2)
    );
    support_to_hips = zof_joint_new("hips", zof_xyz(0,0,0), zof_xyzw(0,1,0,0));
    zof_part_joint_put(support, support_to_hips);
    zof_part_attach(hips, support);
    // Base.
    return zof_part_new_group("base", hips);
}

zof_part head_new(void) {
	zof_part eye_left, neck, skull;
	zof_joint eye_left_to_skull, neck_to_skull, neck_to_torso, skull_to_eye_left, skull_to_neck;
	// Skull.
	skull = zof_part_new_capsule("skull", 0.06, 0.01);
	skull_to_neck = zof_joint_new(
		"neck",
		zof_capsule_end_pos(zof_part_capsule(skull), -1),
		// TODO Rotate around Z for Y sideways? Or just make X the axis of rotation?
		zof_xyzw(0,1,0,0)
	);
	zof_part_joint_put(skull, skull_to_neck);
	// Neck.
	neck = zof_part_new_capsule("neck", 0.04, 0);
	neck_to_torso = zof_joint_new(
		"torso",
		zof_capsule_end_pos(zof_part_capsule(neck), -0.3),
		zof_xyzw(0,1,0,0)
	);
	zof_part_joint_put(neck, neck_to_torso);
	neck_to_skull = zof_joint_new(
		"skull",
		zof_capsule_end_pos(zof_part_capsule(neck), 0.3),
		// TODO Rotate around Z for Y sideways? Or just make X the axis of rotation?
		zof_xyzw(0,1,0,0)
	);
	zof_part_joint_put(neck, neck_to_skull);
	zof_part_attach(skull, neck);
	// Eyes.
	// TODO Consider making eyes a separately defined part.
	eye_left = zof_part_new_capsule("eye_left", 0.015, 0);
	zof_part_material_put(eye_left, zof_material_new(0xFF0060A0,0.001));
	eye_left_to_skull = zof_joint_new("skull", zof_xyz(0,0,0), zof_xyzw(0,1,0,0));
	zof_part_joint_put(eye_left, eye_left_to_skull);
	skull_to_eye_left = zof_joint_new(
		"eye_left",
		zof_capsule_end_pos_ex(zof_part_capsule(skull), 0.8, zof_xyz(-0.2,0,1), 1),
		zof_xyzw(0,1,0,0)
	);
	zof_part_joint_put(skull, skull_to_eye_left);
	zof_part_attach(skull, eye_left);
	zof_part_mirror(eye_left);
	// Return group.
	return zof_part_new_group("head", skull);
}

zof_part humanoid_new(void) {
	zof_part humanoid, torso;
	torso = torso_new();
	zof_part_attach(torso, head_new());
	zof_part_attach(torso, base_wheeled_new());
	humanoid = zof_part_new_group("humanoid", torso);
	zof_part_material_put(humanoid, zof_material_new(0xFF808080,1));
	return humanoid;
}

zof_part torso_new(void) {
	zof_part abdomen, chest;
	zof_joint abdomen_to_base, abdomen_to_chest, chest_to_abdomen, chest_to_head;
	// Chest.
	chest = zof_part_new_capsule("chest", 0.1, 0.0725);
	chest_to_abdomen = zof_joint_new(
		"abdomen",
		zof_capsule_end_pos(zof_part_capsule(chest), -0.5),
		zof_xyzw(0,1,0,0)
	);
	zof_part_joint_put(chest, chest_to_abdomen);
	chest_to_head = zof_joint_new(
		"head",
		zof_capsule_end_pos(zof_part_capsule(chest), 1),
		zof_xyzw(0,1,0,0)
	);
	zof_part_joint_put(chest, chest_to_head);
	// Abdomen.
	abdomen = zof_part_new_capsule("abdomen", 0.08, 0.05);
	abdomen_to_chest = zof_joint_new(
		"chest",
		zof_capsule_end_pos(zof_part_capsule(abdomen), 0.5),
		zof_xyzw(0,1,0,0)
	);
	zof_part_joint_put(abdomen, abdomen_to_chest);
	abdomen_to_base = zof_joint_new(
		"base",
		zof_capsule_end_pos(zof_part_capsule(abdomen), -0.5),
		zof_xyzw(0,1,0,0)
	);
	zof_part_joint_put(abdomen, abdomen_to_base);
	// Attach them.
	zof_part_attach(chest, abdomen);
	return zof_part_new_group("torso", chest);
}

zof_part wheel_new(void) {
	zof_part wheel;
	zof_joint wheel_to_body;
	wheel = zof_part_new_cylinder("wheel", zof_xyz(0.2,0.04,0.2));
	zof_part_material_put(wheel, zof_material_new(0xFF202020, 1));
	wheel_to_body = zof_joint_new("body", zof_part_end_pos(wheel,zof_xyz(0,1,0)), zof_xyzw(0,1,0,0));
	zof_part_joint_put(wheel, wheel_to_body);
	return wheel;
}
