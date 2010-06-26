#include <stdio.h>
#include "zof.h"

zof_bool sim_init(zof_mod mod, zof_sim sim) {
	zof_part box1, box2;
	zof_joint joint1;
	printf("Hello from %s!\n", zof_mod_uri(mod));
	// Box 1.
	box1 = zof_part_new_box("box1", zof_xyz(0.1,0.025,0.1));
	zof_part_pos_put(box1, zof_xyz(0,1,0));
	joint1 = zof_joint_new(
		"box2",
		zof_part_end_pos(box1, zof_xyz(1,0,1)),
		zof_xyzw(1,0,1,zof_pi/4)
	);
	zof_part_material_put(box1, zof_material_new(0xFFFF8000,1));
	// TODO Joint limits.
	zof_part_joint_put(box1, joint1);
	// Box 2.
	box2 = zof_part_new_box("box2", zof_xyz(0.08,0.04,0.08));
	zof_part_joint_put(box2, zof_joint_new(
		"box1",
		zof_part_end_pos(box2, zof_xyz(-1,0,-1)),
		zof_xyzw(0,1,1,zof_pi)
	));
	zof_part_material_put(box2, zof_material_new(0xFF800080,1));
	// Attach the boxes.
	zof_part_attach(box1, box2);
	// Add them.
	zof_sim_part_add(sim, box1);
	return zof_true;
}
