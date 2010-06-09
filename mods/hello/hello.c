#include <stdio.h>
#include "zof.h"

zof_bool sim_init(zof_mod mod, zof_sim sim) {
	printf("Hello from %s!\n", zof_mod_uri(mod));
	// Box 1.
	zof_shape box1 = zof_shape_new_box(zof_xyz(0.1,0.025,0.1));
	zof_part part1 = zof_part_new("box1", box1);
	zof_part_pos_put(part1, zof_xyz(0,1.5,0));
	zof_joint joint = zof_joint_new(
		"box2",
		zof_part_end_pos(part1, zof_xyz(0.9,0,0.9)),
		zof_xyzw(0,1,0,0)
	);
	zof_part_joint_add(part1, joint);
	// Box 2.
	zof_shape box2 = zof_shape_new_box(zof_xyz(0.1,0.025,0.1));
	zof_part part2 = zof_part_new("box2", box2);
	// Joint.
	zof_part_attach(part1, part2);
	// Add them.
	zof_sim_part_add(sim, part1);
	return zof_true;
}
