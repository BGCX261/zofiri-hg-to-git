#ifndef zofiri_sim_h
#define zofiri_sim_h

#include "zofiri.h"
#include <btBulletDynamicsCommon.h>
#include <map>
#include <string>

using namespace std;


namespace zof {

struct BasicPart;
struct GroupPart;
struct Part;

struct Part: Any {

	Part();

	Part(const string& name);

	static Part* of(zof_part part);

	BasicPart* basic();

	const btTransform& getTransform();

	/**
	 * If flood, then search and replace all of the current
	 * material with the given, across attached parts.
	 */
	void setMaterial(Material* material, bool flood=true);

	void setPos(const btVector3& pos);

	/**
	 * Sets the world transform of this part.
	 *
	 * Also applies the same effective relative transform to all
	 * attached parts as if a full rigid body.
	 */
	void setTransform(const btTransform& transform);

	/**
	 * Assumes only trees, not cycles, for recursion.
	 * BasicPart as parent because joints always work at the BasicPart level.
	 */
	void transformBy(const btTransform& relative, BasicPart* parent=0);

	btRigidBody* body;

	/**
	 * This allows only strictly nested groups.
	 * TODO Look into efficient arbitrary cross-groupings sometime.
	 */
	GroupPart* group;

	map<string,zof_joint> joints;

	string name;

private:

	void init();

};

struct BasicPart: Part {

	BasicPart();

	BasicPart(const string& name, btCollisionShape* shape);

	virtual ~BasicPart();

	static zof_export BasicPart* of(btCollisionObject* body);

	static BasicPart* of(zof_box box);

	static BasicPart* of(zof_capsule capsule);

	static BasicPart* of(zof_part part);

	Material* material;

	void* sceneNode;

	Sim* sim;

private:

	void init();

};

struct GroupPart: Part {

	GroupPart(const string& name, Part* root);

	// TODO Destructor to delete whole tree.

	// TODO Access to root part (which could be a group)?

private:

	/**
	 * Recursive function to set groups and find detached joints.
	 */
	void init(BasicPart* part, BasicPart* parent=0);

};

struct Material: Any {

	zof_export Material(zof_color color = 0xFFFFFFFF);

	zof_color color;

	btScalar density;

	// TODO btScalar friction;

	static Material* defaultMaterial();

};

struct MotionState: btDefaultMotionState {

	virtual void setWorldTransform(const btTransform& worldTrans);

};

struct Sim: Any {

	zof_export Sim();

	zof_export ~Sim();

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

	zof_export btRigidBody* createBody(
		btCollisionShape* shape,
		const btTransform& transform,
		Material* material = Material::defaultMaterial()
	);

	zof_export btRigidBody* createPlane();

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
