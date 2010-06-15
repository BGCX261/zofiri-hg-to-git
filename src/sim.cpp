#include "zofiri.h"
#include "mat.h"
#include "sim.h"
#include "viz.h"
#include <string>
#include <vector>


/**
 * Make the core unit centimeters for finer control.
 */
#define zof_bt_scale 100.0

namespace zof {

struct Joint: Any {
	zof_type type;
	zof_str name;
	zof_part part;
	zof_joint other;
	btTransform transform;
	zof_vec4 posLimits[2];
	zof_vec4 rotLimits[2];

	/**
	 * Doable if part, other, and remaining items already set.
	 */
	btTypedConstraint* createConstraint();

};

struct Shape: Any {

	virtual ~Shape() {
		delete shape;
	}

	btCollisionShape* shape;

};

}


using namespace zof;

extern "C" {

zof_vec4 zof_bt3_to_vec4(btVector3 bt3, zof_num scale=1);
btVector3 zof_vec4_to_bt3(zof_vec4 vec, zof_num scale=1);
btVector4 zof_vec4_to_bt4(zof_vec4 vec, zof_num scale=1);

zof_vec4 zof_box_radii(zof_box box) {
	BasicPart* part_struct = (BasicPart*)box;
	btBoxShape* shape = reinterpret_cast<btBoxShape*>(part_struct->body->getCollisionShape());
	btVector3 radii = shape->getHalfExtentsWithMargin();
	return zof_bt3_to_vec4(radii, 1/zof_bt_scale);
}

zof_vec4 zof_bt3_to_vec4(btVector3 bt3, zof_num scale) {
	zof_vec4 vec;
	for (zof_uint i = 0; i < 3; i++) {
		vec.vals[i] = scale * bt3.m_floats[i];
	}
	vec.vals[3] = 0;
	return vec;
}

zof_str zof_joint_name(zof_joint joint) {
	return ((Joint*)joint)->name;
}

zof_joint zof_joint_new(zof_str name, zof_vec4 pos, zof_vec4 rot) {
	// TODO Hrmm.
	Joint* joint = new Joint;
	joint->name = name;
	joint->part = zof_null;
	joint->other = zof_null;
	joint->transform.setOrigin(zof_vec4_to_bt3(pos, zof_bt_scale));
	joint->transform.setRotation(btQuaternion(zof_vec4_to_bt3(rot),btScalar(rot.vals[3])));
	// TODO Or would it be better just to store a partially filled btGeneric6DofConstraint?
	memset(joint->posLimits, 0, sizeof(joint->posLimits));
	memset(joint->rotLimits, 0, sizeof(joint->rotLimits));
	return (zof_joint)joint;
}

zof_joint zof_joint_other(zof_joint joint) {
	return ((Joint*)joint)->other;
}

zof_part zof_joint_part(zof_joint joint) {
	return ((Joint*)joint)->part;
}

zof_bool zof_part_attach(zof_part part, zof_part kid) {
	zof_str part_name = zof_part_name(part);
	zof_joint kid_joint = zof_part_joint(kid, part_name);
	if (kid_joint) {
		zof_str kid_name = zof_part_name(kid);
		zof_joint part_joint = zof_part_joint(part, kid_name);
		if (part_joint) {
			Joint* partJoint = (Joint*)part_joint;
			Joint* kidJoint = (Joint*)kid_joint;
			partJoint->other = kid_joint;
			kidJoint->other = part_joint;
		}
	}
	return zof_false;
}

zof_box zof_part_box(zof_part part) {
	return zof_part_shape_kind(part) == zof_shape_kind_box ? (zof_box)part : zof_null;
}

zof_vec4 zof_part_end_pos(zof_part part, zof_vec4 ratios) {
	// TODO Separate bounds function? What about non-centered origin?
	zof_vec4 radii;
	switch (zof_part_shape_kind(part)) {
	case zof_shape_kind_box:
		radii = zof_box_radii(zof_part_box(part));
		break;
	default:
		// TODO Not easy to indicate error here!
		memset(&radii, 0, sizeof(radii));
		break;
	}
	for (int i = 0; i < 4; i++) {
		radii.vals[i] *= ratios.vals[i];
	}
	return radii;
}

zof_joint zof_part_joint(zof_part part, zof_str name) {
	BasicPart* part_struct = (BasicPart*)part;
	map<string,zof_joint>::iterator j = part_struct->joints.find(name);
	if (j != part_struct->joints.end()) {
		return j->second;
	}
	return zof_null;
}

zof_joint zof_part_joint_put(zof_part part, zof_joint joint) {
	BasicPart* part_struct = (BasicPart*)part;
	string name(zof_joint_name(joint));
	zof_joint old_joint = zof_null;
	map<string,zof_joint>::iterator old = part_struct->joints.find(name);
	if (old != part_struct->joints.end()) {
		// Old joint already there.
		old_joint = old->second;
		Joint* old_joint_struct = (Joint*)old_joint;
		old_joint_struct->part = zof_null;
	}
	map<string,zof_joint>::value_type pair(name, joint);
	part_struct->joints.insert(old, pair);
	Joint* joint_struct = (Joint*)joint;
	joint_struct->part = part;
	return old_joint;
}

zof_str zof_part_name(zof_part part) {
	return const_cast<zof_str>(((BasicPart*)part)->name.c_str());
}

zof_shape_kind zof_part_shape_kind(zof_part part) {
	BasicPart* part_struct = (BasicPart*)part;
	switch(part_struct->body->getCollisionShape()->getShapeType()) {
	case BOX_SHAPE_PROXYTYPE:
		return zof_shape_kind_box;
	case CAPSULE_SHAPE_PROXYTYPE:
		return zof_shape_kind_capsule;
	case CYLINDER_SHAPE_PROXYTYPE:
		return zof_shape_kind_cylinder;
	default:
		return zof_shape_kind_error;
	}
}

zof_part zof_part_new(zof_str name, zof_shape shape) {
	BasicPart* part = new BasicPart;
	part->name = name;
	// TODO This is mostly copied from Sim::createBody.
	// TODO We need to merge this sometime.
	// Actually setting the material will require recalculating mass props.
	Material* material = Material::defaultMaterial();
	btScalar volume = Sim::calcVolume(((Shape*)shape)->shape);
	btScalar mass(material->density * volume);
	btVector3 inertia(0,0,0);
	((Shape*)shape)->shape->calculateLocalInertia(mass, inertia);
	MotionState* motionState = new MotionState();
	btTransform transform;
	transform.setIdentity();
	motionState->m_graphicsWorldTrans = transform;
	btRigidBody::btRigidBodyConstructionInfo bodyConstruct(mass, motionState, ((Shape*)shape)->shape, inertia);
	part->body = new btRigidBody(bodyConstruct);
	part->material = material;
	// Sim will need set when the part is added to the sim.
	// TODO When and how to copy parts? Obviously important need.
	part->sim = zof_null;
	part->body->setUserPointer(part);
	motionState->m_userPointer = part->body;
	return (zof_part)part;
}


zof_part zof_part_new_box(zof_str name, zof_vec4 radii) {
	return zof_part_new(name, zof_shape_new_box(radii));
}

void zof_part_pos_add(zof_part part, zof_vec4 pos) {
	// TODO
}

void zof_part_pos_put(zof_part part, zof_vec4 pos) {
	BasicPart* part_struct = (BasicPart*)part;
	// TODO Always go through attached parts and move them as if rigidly attached.
	// TODO That is, apply the same relative transform to each.
	btVector3 bt = zof_vec4_to_bt3(pos, zof_bt_scale);
	part_struct->body->getWorldTransform().setOrigin(bt);
}

void zof_part_rot_add(zof_part part, zof_vec4 rot) {
	// TODO
}

void zof_part_rot_put(zof_part part, zof_vec4 rot) {
	// TODO
}

zof_shape zof_shape_new_box(zof_vec4 radii) {
	Shape* shape = new Shape;
	btVector3 bt_radii = zof_vec4_to_bt3(radii, zof_bt_scale);
	btBoxShape* box = new btBoxShape(bt_radii);
	shape->shape = box;
	return (zof_shape)shape;
}

//zof_shape zof_shape_new_capsule(zof_num rad_xy, zof_num half_spread);

//zof_shape zof_shape_new_cylinder(zof_vec4 radii);

//zof_shape zof_shape_new_mesh(zof_mesh mesh);

void zof_sim_part_add(zof_sim sim, zof_part part) {
	Sim* simPriv = reinterpret_cast<Sim*>(sim);
	BasicPart* part_struct = reinterpret_cast<BasicPart*>(part);
	// TODO Add full graph whether composite or not?
	// Avoid infinite recursion by checking sim.
	// TODO If in another sim, move to this one?
	if (!part_struct->sim) {
		part_struct->sim = simPriv;
		simPriv->addBody(part_struct->body);
		for (map<string,zof_joint>::iterator j = part_struct->joints.begin(); j != part_struct->joints.end(); j++) {
			zof_joint joint = j->second;
			zof_joint other = zof_joint_other(joint);
			if (other) {
				zof_part kid = zof_joint_part(other);
				zof_sim_part_add(sim, kid);
				// TODO Reset relative positions somewhere!
				simPriv->addConstraint(((Joint*)joint)->createConstraint());
			}
		}
	}
}

btVector3 zof_vec4_to_bt3(zof_vec4 vec, zof_num scale) {
	btVector3 bt;
	for (zof_uint i = 0; i < 3; i++) {
		bt.m_floats[i] = btScalar(scale * vec.vals[i]);
	}
	bt.m_floats[3] = btScalar(0);
	return bt;
}

btVector4 zof_vec4_to_bt4(zof_vec4 vec, zof_num scale) {
	btVector4 bt;
	for (zof_uint i = 0; i < 4; i++) {
		bt.m_floats[i] = btScalar(scale * vec.vals[i]);
	}
	return bt;
}

}


/**
 * Older stuff.
 * TODO Eventually merge with newer.
 */
namespace zof {

BasicPart::BasicPart():
		body(0),
		material(Material::defaultMaterial()),
		sceneNode(0),
		sim(0) {
	// Nothing more to do.
}

BasicPart::~BasicPart() {
	// TODO Remove it from the sim first, along with constraints?
	// TODO Remove attached parts?
	delete body;
}

BasicPart* BasicPart::of(btCollisionObject* body) {
	return reinterpret_cast<BasicPart*>(body->getUserPointer());
}

btTypedConstraint* Joint::createConstraint() {
	BasicPart* part = reinterpret_cast<BasicPart*>(this->part);
	Joint* other = reinterpret_cast<Joint*>(this->other);
	BasicPart* otherPart = reinterpret_cast<BasicPart*>(other->part);
	btGeneric6DofConstraint* constraint = new btGeneric6DofConstraint(*part->body, *otherPart->body, this->transform, other->transform, false);
	// TODO Limits, etc.
	return constraint;
}

Material::Material(zof_color c): color(c), density(1) {
	// Nothing more to do.
}

Material* Material::defaultMaterial() {
	static Material* material = new Material;
	return material;
}

void MotionState::setWorldTransform(const btTransform& worldTransform) {
	btDefaultMotionState::setWorldTransform(worldTransform);
	btRigidBody* body = reinterpret_cast<btRigidBody*>(m_userPointer);
	BasicPart* info = BasicPart::of(body);
	Sim* sim = info->sim;
	//	cout << "UPDATE: ";
	//	cout << "body = " << body << "; ";
	//	cout << "info = " << info << "; ";
	//	cout << "info->sim = " << info->sim << "; ";
	//	cout << "body->getUserPointer() = " << body->getUserPointer() << "; ";
	//	cout << "motionState = " << this << "; ";
	//	cout << "motionState->m_userPointer = " << m_userPointer << "\n";
	if(sim->viz) {
		sim->viz->update(body);
	}
}

Sim::Sim() {
	dispatcher = new btCollisionDispatcher(&collisionConfiguration);
	dynamics = new btDiscreteDynamicsWorld(dispatcher, &broadphase, &solver, &collisionConfiguration);
	unitsRatio = 100;
	viz = 0;
	// Weaken gravity a bit. Copes this way better at larger ratios.
	dynamics->setGravity(0.1 * unitsRatio * dynamics->getGravity());
}

Sim::~Sim() {
	// TODO delete bodies, constraints, materials, shapes, ...
	delete dynamics;
	delete dispatcher;
}

int Sim::addBody(btRigidBody* body) {
	int id = generateId();
	bodies[id] = body;
	dynamics->addRigidBody(body);
	if (viz) {
		viz->addBody(body);
	}
	return id;
}

int Sim::addConstraint(btTypedConstraint* constraint) {
	int id = generateId();
	constraints[id] = constraint;
	// TODO Allow option for collisions between linked bodies?
	dynamics->addConstraint(constraint, true);
	return id;
}

int Sim::addMaterial(Material* material) {
	int id = generateId();
	materials[id] = material;
	return id;
}

int Sim::addShape(btCollisionShape* shape) {
	int id = generateId();
	shapes[id] = shape;
	return id;
}

btScalar Sim::calcVolume(btCollisionShape* shape) {
	// Just stick to internal units here for now.
	// Let people convert outside as needed.
	switch(shape->getShapeType()) {
	case BOX_SHAPE_PROXYTYPE:
		return calcVolumeBox(reinterpret_cast<btBoxShape*>(shape));
	case CAPSULE_SHAPE_PROXYTYPE:
		return calcVolumeCapsule(reinterpret_cast<btCapsuleShape*>(shape));
	case CYLINDER_SHAPE_PROXYTYPE:
		return calcVolumeCylinder(reinterpret_cast<btCylinderShape*>(shape));
	case STATIC_PLANE_PROXYTYPE:
		// TODO Maybe NaN would be better.
		return 0;
	case SPHERE_SHAPE_PROXYTYPE:
		return calcVolumeSphere(reinterpret_cast<btSphereShape*>(shape));
	default:
		throw "unsupported shape kind";
	}
}

btScalar Sim::calcVolumeBox(btBoxShape* shape) {
	btVector3 extents = 2 * shape->getHalfExtentsWithMargin();
	return extents.x() * extents.y() * extents.z();
}

btScalar Sim::calcVolumeCapsule(btCapsuleShape* shape) {
	// TODO Support scaling!
	btScalar radius = shape->getRadius();
	btScalar sphereVol = pi(4.0/3.0) * pow(radius, 3);
	btScalar cylinderVol = pi(2.0) * radius * radius * shape->getHalfHeight();
	return sphereVol + cylinderVol;
}

btScalar Sim::calcVolumeCylinder(btCylinderShape* shape) {
	btVector3 halfExtents = shape->getHalfExtentsWithMargin();
	return pi(1.0) * halfExtents.x() * halfExtents.z() * 2 * halfExtents.y();
}

btScalar Sim::calcVolumeSphere(btSphereShape* shape) {
	return pi(4.0/3.0) * pow(shape->getRadius(), 3);
}

btRigidBody* Sim::createBody(btCollisionShape* shape, const btTransform& transform, Material* material) {
	// TODO Scale the shape based on the simulation scale. How to define?
	// TODO Make a common helper function for following stuff!
	// TODO We really need volume calculations for density!
	btScalar volume = calcVolume(shape);
	btScalar mass(material->density * volume);
	btVector3 inertia(0,0,0);
	shape->calculateLocalInertia(mass, inertia);
	// TODO itemSub->transform.setIdentity();
	// TODO Make a different MotionState to handle callback updates.
	MotionState* motionState = new MotionState();
	motionState->m_graphicsWorldTrans = transform;
	btRigidBody::btRigidBodyConstructionInfo bodyConstruct(mass, motionState, shape, inertia);
	btRigidBody* body = new btRigidBody(bodyConstruct);
	BasicPart* info = new BasicPart;
	info->material = material;
	info->sim = this;
	body->setUserPointer(info);
	motionState->m_userPointer = body;
	//	cout << "START: ";
	//	cout << "body = " << body << "; ";
	//	cout << "info = " << info << "; ";
	//	cout << "info->sim = " << info->sim << "; ";
	//	cout << "body->getUserPointer() = " << body->getUserPointer() << "; ";
	//	cout << "motionState = " << motionState << "; ";
	//	cout << "motionState->m_userPointer = " << motionState->m_userPointer << "\n";
	return body;
}

btRigidBody* Sim::createPlane() {
	// TODO Scale the shape based on the simulation scale.
	btCollisionShape* shape = new btStaticPlaneShape(btVector3(0,1,0), 0);
	// Zero mass flags static. TODO Other changes needed? Can I get away with non-rigid body?
	btScalar mass(0);
	btVector3 inertia(0,0,0);
	MotionState* motionState = new MotionState();
	btRigidBody::btRigidBodyConstructionInfo bodyConstruct(mass, motionState, shape, inertia);
	btRigidBody* body = new btRigidBody(bodyConstruct);
	BasicPart* info = new BasicPart;
	info->sim = this;
	body->setUserPointer(info);
	motionState->m_userPointer = body;
	return body;
}

int Sim::generateId() {
	// TODO We can do better than this.
	return rand();
}

btRigidBody* Sim::getBody(int id) {
	std::map<int,btRigidBody*>::iterator b = bodies.find(id);
	return b == bodies.end() ? 0 : b->second;
}

btTypedConstraint* Sim::getConstraint(int id) {
	std::map<int,btTypedConstraint*>::iterator c = constraints.find(id);
	return c == constraints.end() ? 0 : c->second;
}

Material* Sim::getMaterial(int id) {
	std::map<int,Material*>::iterator m = materials.find(id);
	return m == materials.end() ? 0 : m->second;
}

btCollisionShape* Sim::getShape(int id) {
	std::map<int,btCollisionShape*>::iterator s = shapes.find(id);
	return s == shapes.end() ? 0 : s->second;
}

}
