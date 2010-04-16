#include "zofiri.h"

#ifndef zofiri_viz_h
#define zofiri_viz_h

namespace zofiri {

struct Viz {

	Viz(Sim* sim);

	IMesh* buildMesh(btRigidBody* body);

	IMesh* buildBoxMesh(btBoxShape* shape, Material* material);

	IMesh* createCapsuleMesh(btCapsuleShape* shape, Material* material, u32 latCount, u32 longCount);

	IMesh* createPlaneMesh();

	/**
	 * Generate an Irrlicht scene node for a sphere.
	 * TODO Better would be to have more longs nearer the equator.
	 */
	IMesh* createSphereMesh(btSphereShape* shape, u32 latCount, u32 longCount);

	/**
	 * Lives only during the life of run.
	 */
	IrrlichtDevice* device;

	void update(btRigidBody* body);

	void run();

	ISceneManager* scene() {
		return device->getSceneManager();
	}

	/**
	 * Not disposed.
	 */
	Sim* sim;

	IVideoDriver* video() {
		return device->getVideoDriver();
	}

};

}

#endif
