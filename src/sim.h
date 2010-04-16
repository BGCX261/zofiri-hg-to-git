#include "zofiri.h"

#ifndef zofiri_sim_h
#define zofiri_sim_h

namespace zofiri {

struct BodyInfo {

	BodyInfo();

	static BodyInfo* of(btCollisionObject* body);

	Material* material;

	ISceneNode* sceneNode;

	Sim* sim;

};

struct Material {

	Material(SColor color = SColor(0xFFFFFFFF));

	SColor color;

	btScalar density;

	// TODO btScalar friction;

	static Material* defaultMaterial();

};

struct MotionState: btDefaultMotionState {

	virtual void setWorldTransform(const btTransform& worldTrans);

};

struct Sim {

	Sim();

	~Sim();

	btDbvtBroadphase broadphase;

	btScalar calcVolume(btCollisionShape* shape);

	btScalar calcVolumeBox(btBoxShape* shape);

	btScalar calcVolumeCapsule(btCapsuleShape* shape);

	btScalar calcVolumeSphere(btSphereShape* shape);

	btDefaultCollisionConfiguration collisionConfiguration;

	btRigidBody* createBody(
		btCollisionShape* shape,
		const btTransform& transform,
		Material* material = Material::defaultMaterial()
	);

	btRigidBody* createPlane();

	btCollisionDispatcher* dispatcher;

	btDiscreteDynamicsWorld* dynamics;

	btSequentialImpulseConstraintSolver solver;

	/**
	 * Not disposed.
	 */
	Viz* viz;

};

}

#endif
