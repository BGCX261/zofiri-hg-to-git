#include <stdio.h>
#include "zof.h"

zof_part torso_new(void);

zof_bool sim_init(zof_mod mod, zof_sim sim) {
	zof_sim_part_add(sim, torso_new());
	return zof_true;
}

zof_part torso_new(void) {
	//
	//zof_
	return zof_null;
}
