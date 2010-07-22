#ifndef zofiri_sim_h
#define zofiri_sim_h

#include "zofiri.h"
#include <btBulletDynamicsCommon.h>
#include <map>
#include <string>
#include <vector>

using namespace std;


namespace zof {

struct BasicPart;
struct GroupPart;
struct Joint;
struct Part;
struct Updater;

struct Part: Any {

	Part();

	Part(const string& name);

	static Part* of(zofPart part);

	zofPart asC();

	/**
	 * Returns true of successfully attached.
	 */
	bool attach(Part* other, bool swap = false);

	BasicPart* basic();

	virtual Part* copyTo(const btVector3& pos, const string& oldSub, const string& newSub) = 0;

	virtual void extents(btVector3* min, btVector3* max) = 0;

	const btTransform& getTransform();

	bool inGroup(GroupPart* group);

	Joint* joint(const string& name);

	/**
	 * Returns the old joint if any.
	 */
	Joint* jointPut(Joint* joint);

	virtual Part* mirror() = 0;

	/**
	 * Return the child part or descendent if starts with "//".
	 */
	Part* part(const string& name);

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

	map<string,Joint*> joints;

	string name;

private:

	void init();

};

struct BasicPart: Part {

	BasicPart();

	BasicPart(const string& name, btCollisionShape* shape);

	virtual ~BasicPart();

	static zofExport BasicPart* of(btCollisionObject* body);

	static BasicPart* of(zofBox box);

	static BasicPart* of(zofCapsule capsule);

	static BasicPart* of(zofPart part);

	virtual Part* copyTo(const btVector3& pos, const string& oldSub, const string& newSub);

	virtual void extents(btVector3* min, btVector3* max);

	virtual Part* mirror();

	Material* material;

	void* sceneNode;

	Sim* sim;

private:

	void init();

};

struct GroupPart: Part {

	GroupPart(const string& name, Part* root);

	// TODO Destructor to delete whole tree.

	virtual Part* copyTo(const btVector3& pos, const string& oldSub, const string& newSub);

	virtual void extents(btVector3* min, btVector3* max);

	virtual Part* mirror();

	// TODO Access to root part (which could be a group)?

private:

	/**
	 * Recursive function to set groups and find detached joints.
	 */
	void init(BasicPart* part, BasicPart* parent=0);

};

struct Material: Any {

	zofExport Material(zofColor color = 0xFFFFFFFF);

	static Material* defaultMaterial();

	static Material* of(zofMaterial material);

	zofMaterial asC();

	zofColor color;

	btScalar density;

	// TODO btScalar friction;

};

struct MotionState: btDefaultMotionState {

	virtual void setWorldTransform(const btTransform& worldTrans);

};

struct Sim: Any {

	zofExport Sim();

	zofExport ~Sim();

	static Sim* of(zofSim sim);

	int addBody(btRigidBody* body);

	int addConstraint(btTypedConstraint* constraint);

	int addMaterial(Material* material);

	int addShape(btCollisionShape* body);

	zofSim asC();

	static btScalar calcVolume(btCollisionShape* shape);

	static btScalar calcVolumeBox(btBoxShape* shape);

	static btScalar calcVolumeCapsule(btCapsuleShape* shape);

	static btScalar calcVolumeCylinder(btCylinderShape* shape);

	static btScalar calcVolumeSphere(btSphereShape* shape);

	/**
	 * Converts the value in centimeters to whatever unit we use internally.
	 * This relates to needs perhaps in Bullet to scale differently depending
	 * on the precision we need. We could perhaps get even fancier in the
	 * future, too, but this at least helps tag units.
	 */
	template<typename Num> Num cm(Num centimeters) {
		return m(0.01 * centimeters);
	}

	zofExport btRigidBody* createBody(
		btCollisionShape* shape,
		const btTransform& transform,
		Material* material = Material::defaultMaterial()
	);

	zofExport btRigidBody* createPlane();

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

	void update();

	/**
	 * Mapped for protocol access.
	 * TODO btCollisionObject or btRigidBody? Can just say 'bodies' either way, especially since 'objects' is vague.
	 */
	map<int, btRigidBody*> bodies;

	btDbvtBroadphase broadphase;

	btDefaultCollisionConfiguration collisionConfiguration;

	/**
	 * Mapped for protocol access.
	 */
	map<int, btTypedConstraint*> constraints;

	btCollisionDispatcher* dispatcher;

	btDiscreteDynamicsWorld* dynamics;

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

	vector<Updater*> updaters;

	/**
	 * Not disposed.
	 */
	Viz* viz;

};

struct Updater {
	virtual void update(Sim* sim) = 0;
};

}

#endif
