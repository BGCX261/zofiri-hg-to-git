#ifndef zofiri_sim_h
#define zofiri_sim_h

#include "zofiri.h"

namespace zof {

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

	/**
	 * Converts the value in centimeters to whatever unit we use internally.
	 * This relates to needs perhaps in Bullet to scale differently depending
	 * on the precision we need. We could perhaps get even fancier in the
	 * future, too, but this at least helps tag units.
	 */
	template<typename Num> Num cm(Num centimeters) {
		return m(0.01 * centimeters);
	}

	btRigidBody* createBody(
		btCollisionShape* shape,
		const btTransform& transform,
		Material* material = Material::defaultMaterial()
	);

	btRigidBody* createPlane();

	/**
	 * Converts the value in meters to whatever unit we use internally.
	 * This relates to needs perhaps in Bullet to scale differently depending
	 * on the precision we need. We could perhaps get even fancier in the
	 * future, too, but this at least helps tag units.
	 */
	template<typename Num> Num m(Num meters) {
		return Num(unitsRatio * meters);
	}

	btCollisionDispatcher* dispatcher;

	btDiscreteDynamicsWorld* dynamics;

	btSequentialImpulseConstraintSolver solver;

	btScalar unitsRatio;

	/**
	 * Not disposed.
	 */
	Viz* viz;

};

}

#endif
