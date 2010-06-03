#ifndef zofiri_viz_h
#define zofiri_viz_h

#include "zofiri.h"

namespace zof {

struct Viz {

	Viz(Sim* sim);

	void addBody(btCollisionObject* body);

	IMesh* buildBoxMesh(btBoxShape* shape, Material* material);

	/**
	 * TODO Change to supports only Y-axis capsules.
	 */
	IMesh* createCapsuleMesh(btCapsuleShape* shape, Material* material, u32 latCount, u32 longCount);

	/**
	 * Supports only Y-axis cylinders.
	 */
	IMesh* createCylinderMesh(btCylinderShape* shape, Material* material, u32 longCount);

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

	/**
	 * Not disposed.
	 */
	Pub* pub;

	void run();

	ISceneManager* scene() {
		return device->getSceneManager();
	}

	/**
	 * Not disposed.
	 */
	Sim* sim;

	void update(btRigidBody* body);

	IVideoDriver* video() {
		return device->getVideoDriver();
	}

};

}

#endif
