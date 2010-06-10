#include <stdio.h>
#include "zof.h"

zof_bool sim_init(zof_mod mod, zof_sim sim) {
	printf("Hello from %s!\n", zof_mod_uri(mod));
	// Box 1.
	zof_part box1 = zof_part_new_box("box1", zof_xyz(0.1,0.025,0.1));
	zof_part_pos_put(box1, zof_xyz(0,1.2,0));
	zof_joint joint1 = zof_joint_new(
		"box2",
		zof_part_end_pos(box1, zof_xyz(0.9,0,0.9)),
		zof_xyzw(0,1,0,0)
	);
	// TODO Joint limits.
	zof_part_joint_add(box1, joint1);
	// Box 2.
	zof_part box2 = zof_part_new_box("box2", zof_xyz(0.1,0.025,0.1));
	zof_part_joint_add(box2, zof_joint_new(
		"box1",
		zof_part_end_pos(box2, zof_xyz(0.9,0,0.9)),
		zof_xyzw(0,1,0,0)
	));
	// Attach the boxes.
	zof_part_attach(box1, box2);
	// Add them.
	zof_sim_part_add(sim, box1);
	return zof_true;
}
