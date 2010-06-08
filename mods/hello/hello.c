#include <stdio.h>
#include <zof.h>

zof_bool world_init(zof_mod mod, zof_world world) {
	printf("Hello from %s!\n", zof_mod_uri(mod));
	return zof_true;
}
