#ifndef zofiri_viz_h
#define zofiri_viz_h

#include "zofiri.h"
#include <btBulletDynamicsCommon.h>

namespace zof {

/**
 * Quick hack to get irrlicht.h away from elsewhere.
 * Replace with C interface soon.
 */
struct Viz {

	static zof_export Viz* create(Sim* sim);

	virtual ~Viz() {
		// Nothing to do.
	}

	virtual void addBody(btCollisionObject* body) = 0;

	virtual void run() = 0;

	/**
	 * Not disposed.
	 */
	Pub* pub;

	/**
	 * Not disposed.
	 */
	Sim* sim;

	virtual void update(btRigidBody* body) = 0;

};

}

#endif
