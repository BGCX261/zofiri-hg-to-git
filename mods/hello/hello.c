#include <stdio.h>
#include <zof.h>

zof_bool sim_init(zof_mod mod, zof_sim sim) {
	printf("Hello from %s!\n", zof_mod_uri(mod));
	return zof_true;
}
