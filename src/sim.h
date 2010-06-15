#ifndef zofiri_sim_h
#define zofiri_sim_h

#include "zofiri.h"
#include <btBulletDynamicsCommon.h>
#include <map>


namespace zof {

struct BodyInfo {

	BodyInfo();

	static BodyInfo* of(btCollisionObject* body);

	Material* material;

	void* sceneNode;

	Sim* sim;

};

struct Material {

	Material(zof_color color = 0xFFFFFFFF);

	zof_color color;

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

	int addBody(btRigidBody* body);

	int addConstraint(btTypedConstraint* constraint);

	int addMaterial(Material* material);

	int addShape(btCollisionShape* body);

	/**
	 * Mapped for protocol access.
	 * TODO btCollisionObject or btRigidBody? Can just say 'bodies' either way, especially since 'objects' is vague.
	 */
	map<int, btRigidBody*> bodies;

	btDbvtBroadphase broadphase;

	static btScalar calcVolume(btCollisionShape* shape);

	static btScalar calcVolumeBox(btBoxShape* shape);

	static btScalar calcVolumeCapsule(btCapsuleShape* shape);

	static btScalar calcVolumeCylinder(btCylinderShape* shape);

	static btScalar calcVolumeSphere(btSphereShape* shape);

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

	/**
	 * Mapped for protocol access.
	 */
	std::map<int, btTypedConstraint*> constraints;

	/**
	 * TODO Delete this once we get it right (obscuring Bullet, etc.).
	 */
	zof_sim csim;

	btRigidBody* createBody(
		btCollisionShape* shape,
		const btTransform& transform,
		Material* material = Material::defaultMaterial()
	);

	btRigidBody* createPlane();

	btCollisionDispatcher* dispatcher;

	btDiscreteDynamicsWorld* dynamics;

	/**
	 * Required to generate an ID that hasn't been generated yet.
	 * Semirandom values might be created, but there's no guarantee of that,
	 * and there's especially no guarantee of strong random values.
	 * TODO We don't really guarantee uniqueness yet. We need to control our own seed.
	 */
	int generateId();

	btRigidBody* getBody(int id);

	btTypedConstraint* getConstraint(int id);

	Material* getMaterial(int id);

	btCollisionShape* getShape(int id);

	/**
	 * Converts the value in meters to whatever unit we use internally.
	 * This relates to needs perhaps in Bullet to scale differently depending
	 * on the precision we need. We could perhaps get even fancier in the
	 * future, too, but this at least helps tag units.
	 */
	template<typename Num> Num m(Num meters) {
		return Num(unitsRatio * meters);
	}

	/**
	 * Mapped for protocol access.
	 */
	map<int, Material*> materials;

	/**
	 * Mapped for protocol access.
	 */
	map<int, btCollisionShape*> shapes;

	btSequentialImpulseConstraintSolver solver;

	btScalar unitsRatio;

	/**
	 * Not disposed.
	 */
	Viz* viz;

};

}

#endif
