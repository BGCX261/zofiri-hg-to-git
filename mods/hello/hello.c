#include <stdio.h>
#include "zof.h"

zof_bool sim_init(zof_mod mod, zof_sim sim) {
	printf("Hello from %s!\n", zof_mod_uri(mod));
	zof_shape box = zof_shape_new_box(zof_xyz(0.1,0.025,0.1));
	zof_part part = zof_part_new("box", box);
	zof_part_pos_put(part, zof_xyz(0,1.5,0));
	zof_sim_part_add(sim, part);
	return zof_true;
}
