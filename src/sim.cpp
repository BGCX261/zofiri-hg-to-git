#include "zofiri.h"
#include "mat.h"
#include "sim.h"
#include "viz.h"
#include <iostream>
#include <string>
#include <vector>


/**
 * Make the core unit centimeters for finer control.
 */
#define zof_bt_scale 100.0

namespace zof {

struct Joint: Any {

	/**
	 * Doable if part, other, and remaining items already set.
	 */
	btGeneric6DofConstraint* createConstraint();

	static Joint* of(zof_joint joint) {
		return reinterpret_cast<Joint*>(joint);
	}

	zof_str name;
	zof_part part;
	zof_joint other;
	btTransform transform;
	zof_vec4 posLimits[2];
	zof_vec4 rotLimits[2];

};

ostream& operator<<(ostream& out, const btVector3& vec) {
	return out << "(" << vec.m_floats[0] << "," << vec.m_floats[1] << "," << vec.m_floats[2] << ")";
}

ostream& operator<<(ostream& out, const btTransform& transform) {
	btVector3 axis = transform.getRotation().getAxis();
	out << "(rot:(" << axis << "," << transform.getRotation().getAngle() << "),";
	btVector3 pos = transform.getOrigin();
	out << "pos:" << pos << ")";
	return out;
}

}


using namespace zof;

extern "C" {

zof_vec4 zof_bt3_to_vec4(btVector3 bt3, zof_num scale=1);
btVector3 zof_vec4_to_bt3(zof_vec4 vec, zof_num scale=1);
btVector4 zof_vec4_to_bt4(zof_vec4 vec, zof_num scale=1);

zof_vec4 zof_box_radii(zof_box box) {
	btBoxShape* shape = reinterpret_cast<btBoxShape*>(BasicPart::of(box)->body->getCollisionShape());
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

zof_vec4 zof_capsule_end_pos(zof_capsule capsule, zof_num radius_ratio) {
	return zof_capsule_end_pos_ex(capsule, radius_ratio, zof_xyz(0,1,0), 1);
}

zof_vec4 zof_capsule_end_pos_ex(
	zof_capsule capsule,
	zof_num radius_ratio,
	zof_vec4 axis,
	zof_num half_spread_ratio
) {
	btVector3 bt_axis = zof_vec4_to_bt3(axis);
	btCapsuleShape* shape = reinterpret_cast<btCapsuleShape*>(
		BasicPart::of(capsule)->body->getCollisionShape()
	);
    if (radius_ratio < 0) {
        bt_axis.m_floats[1] = -bt_axis.m_floats[1];
    }
    radius_ratio = fabs(radius_ratio);
    btVector3 origin = half_spread_ratio * btVector3(0, shape->getHalfHeight(), 0);
    if (bt_axis.getY() < 0) {
        origin = -origin;
    }
    origin += bt_axis.normalize() * radius_ratio * shape->getRadius();
    //cerr << BasicPart::of(capsule)->name << " end: " << origin << endl;
    return zof_bt3_to_vec4(origin, 1/zof_bt_scale);
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
	joint->transform.setIdentity();
	joint->transform.setRotation(btQuaternion(zof_vec4_to_bt3(rot),btScalar(rot.vals[3])));
	joint->transform.setOrigin(zof_vec4_to_bt3(pos,zof_bt_scale));
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

zof_material zof_material_new(zof_color color, zof_num density) {
	Material* material = new Material(color);
	material->density = btScalar(density);
	return reinterpret_cast<zof_material>(material);
}

zof_bool zof_part_attach(zof_part part, zof_part kid) {
	zof_str part_name = zof_part_name(part);
	zof_joint kid_joint = zof_part_joint(kid, part_name);
	if (kid_joint) {
		zof_str kid_name = zof_part_name(kid);
		zof_joint part_joint = zof_part_joint(part, kid_name);
		if (part_joint) {
			// TODO Update pos/rot of kid.
			Joint* partJoint = Joint::of(part_joint);
			Joint* kidJoint = Joint::of(kid_joint);
			btTransform transform = BasicPart::of(part)->getTransform();
			//cerr << "part (" << zof_part_name(part) << "): " << transform << endl;
			btTransform relTransform = partJoint->transform * kidJoint->transform.inverse();
			//cerr << "relTransform: " << relTransform << endl;
			transform *= relTransform;
			//cerr << "kid: " << transform << endl;
			BasicPart::of(kid)->setTransform(transform);
			// Now actually attach joints.
			partJoint->other = kid_joint;
			kidJoint->other = part_joint;
		}
	}
	return zof_false;
}

zof_box zof_part_box(zof_part part) {
	return zof_part_shape_kind(part) == zof_shape_kind_box ? (zof_box)part : zof_null;
}

zof_capsule zof_part_capsule(zof_part part) {
	return zof_part_shape_kind(part) == zof_shape_kind_capsule ? (zof_capsule)part : zof_null;
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
	Part* part_struct = Part::of(part);
	map<string,zof_joint>::iterator j = part_struct->joints.find(name);
	if (j != part_struct->joints.end()) {
		return j->second;
	}
	return zof_null;
}

zof_joint zof_part_joint_put(zof_part part, zof_joint joint) {
	// Or just disallow all non-BasicParts? Here we add to the root.
	BasicPart* part_struct = BasicPart::of(part);
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

void zof_part_material_put(zof_part part, zof_material material) {
	// TODO How and when to free existing?
	BasicPart::of(part)->material = reinterpret_cast<Material*>(material);
}

zof_str zof_part_name(zof_part part) {
	return const_cast<zof_str>(Part::of(part)->name.c_str());
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

zof_part zof_part_new_box(zof_str name, zof_vec4 radii) {
	btVector3 bt_radii = zof_vec4_to_bt3(radii, zof_bt_scale);
	BasicPart* part = new BasicPart(name, new btBoxShape(bt_radii));
	return reinterpret_cast<zof_part>(part);
}

zof_part zof_part_new_capsule(zof_str name, zof_num radius, zof_num half_spread) {
	BasicPart* part = new BasicPart(name, new btCapsuleShape(radius*zof_bt_scale,2*half_spread*zof_bt_scale));
	return reinterpret_cast<zof_part>(part);
}

zof_part zof_part_new_group(zof_str name, zof_part root) {
	return reinterpret_cast<zof_part>(new GroupPart(name, Part::of(root)));
}

void zof_part_pos_add(zof_part part, zof_vec4 pos) {
	// TODO
}

void zof_part_pos_put(zof_part part, zof_vec4 pos) {
	Part::of(part)->setPos(zof_vec4_to_bt3(pos, zof_bt_scale));
}

void zof_part_rot_add(zof_part part, zof_vec4 rot) {
	// TODO
}

void zof_part_rot_put(zof_part part, zof_vec4 rot) {
	// TODO
}

void zof_sim_part_add(zof_sim sim, zof_part part) {
	Sim* simPriv = reinterpret_cast<Sim*>(sim);
	BasicPart* part_struct = BasicPart::of(part);
	// Add full graph whether composite or not.
	// Avoid infinite recursion by checking sim.
	// TODO If in another sim, move to this one?
	if (!part_struct->sim) {
		part_struct->sim = simPriv;
		simPriv->addBody(part_struct->body);
		//cerr << zof_part_name(part) << " #joints: " << part_struct->joints.size() << endl;
		for (map<string,zof_joint>::iterator j = part_struct->joints.begin(); j != part_struct->joints.end(); j++) {
			zof_joint joint = j->second;
			zof_joint other = zof_joint_other(joint);
			if (other) {
				zof_part kid = zof_joint_part(other);
				//cerr << "Found attached " << zof_part_name(kid) << endl;
				zof_sim_part_add(sim, kid);
				btGeneric6DofConstraint* constraint = Joint::of(joint)->createConstraint();
				// TODO Max the mins of joint and other, and min the maxes.
				constraint->setAngularLowerLimit(zof_vec4_to_bt3(Joint::of(joint)->rotLimits[0]));
				constraint->setAngularUpperLimit(zof_vec4_to_bt3(Joint::of(joint)->rotLimits[1]));
				// TODO Pos limits.
				simPriv->addConstraint(constraint);
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

BasicPart::BasicPart() {
	init();
}

BasicPart::BasicPart(const string& name, btCollisionShape* shape): Part(name) {
	init();
	// TODO This is mostly copied from Sim::createBody.
	// TODO We need to merge this sometime.
	// Actually setting the material will require recalculating mass props.
	//Material* material = Material::defaultMaterial();
	btScalar volume = Sim::calcVolume(shape);
	btScalar mass(material->density * volume);
	btVector3 inertia(0,0,0);
	shape->calculateLocalInertia(mass, inertia);
	MotionState* motionState = new MotionState();
	btTransform transform;
	transform.setIdentity();
	motionState->m_graphicsWorldTrans = transform;
	btRigidBody::btRigidBodyConstructionInfo bodyConstruct(mass, motionState, shape, inertia);
	body = new btRigidBody(bodyConstruct);
	// Sim will need set when the part is added to the sim.
	// TODO When and how to copy parts? Obviously important need.
	sim = zof_null;
	body->setUserPointer(this);
	motionState->m_userPointer = body;
}

BasicPart::~BasicPart() {
	// TODO Remove it from the sim first, along with constraints?
	// TODO Remove attached parts?
	delete body;
}

void BasicPart::init() {
	material = Material::defaultMaterial();
	sceneNode = 0;
	sim = 0;
}

BasicPart* BasicPart::of(btCollisionObject* body) {
	return reinterpret_cast<BasicPart*>(body->getUserPointer());
}

BasicPart* BasicPart::of(zof_box box) {
	return reinterpret_cast<BasicPart*>(box);
}

BasicPart* BasicPart::of(zof_capsule capsule) {
	return reinterpret_cast<BasicPart*>(capsule);
}

BasicPart* BasicPart::of(zof_part part) {
	return Part::of(part)->basic();
}

btGeneric6DofConstraint* Joint::createConstraint() {
	BasicPart* part = BasicPart::of(this->part);
	Joint* other = Joint::of(this->other);
	BasicPart* otherPart = BasicPart::of(other->part);
	btGeneric6DofConstraint* constraint = new btGeneric6DofConstraint(*part->body, *otherPart->body, this->transform, other->transform, false);
	// TODO Limits, etc.
	return constraint;
}

GroupPart::GroupPart(const string& name, Part* root): Part(name) {
	body = root->basic()->body;
	init(root->basic());
}

void GroupPart::init(BasicPart* part, BasicPart* parent) {
	// Recurse around, finding detached joints.
	cerr << "init " << name << " at " << part->name << endl;
	map<string, zof_joint>& joints = part->joints;
	for (map<string,zof_joint>::iterator j = joints.begin(); j != joints.end(); j++) {
		zof_joint joint = j->second;
		zof_joint other = zof_joint_other(joint);
		if (other) {
			BasicPart* kid = BasicPart::of(zof_joint_part(other));
			if (kid != parent) {
				init(kid, part);
			}
		} else {
			// Add detached joints to the group for nice access.
			// This assumes (somewhat fairly) that they'll have unique names.
			cerr << "adding joint to " << zof_joint_name(joint) << endl;
			this->joints[zof_joint_name(joint)] = joint;
		}
	}
	// TODO Go up the group chain until none set, then set to this.
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

Part::Part() {
	init();
}

Part::Part(const string& name) {
	this->name = name;
	init();
}

BasicPart* Part::basic() {
	return BasicPart::of(body);
}

const btTransform& Part::getTransform() {
	return body->getWorldTransform();
}

void Part::init() {
	body = 0;
	group = 0;
}

Part* Part::of(zof_part part) {
	return reinterpret_cast<Part*>(part);
}

void Part::setPos(const btVector3& pos) {
	btTransform transform(getTransform().getRotation(), pos);
	setTransform(transform);
}

void Part::setTransform(const btTransform& transform) {
	// Creating a relative transform then applying that provides room for error.
	// But it keeps the whole flow simpler, and I'm prefer that for now.
	btTransform relative(getTransform().inverseTimes(transform));
	transformBy(relative);
}

void Part::transformBy(const btTransform& relative, BasicPart* parent) {
	//cerr << "Moving " << name << " from " << body->getWorldTransform();
	body->getWorldTransform() *= relative;
	//cerr << " to " << body->getWorldTransform() << " by " << relative << endl;
	for (map<string,zof_joint>::iterator j = joints.begin(); j != joints.end(); j++) {
		zof_joint joint = j->second;
		zof_joint other = zof_joint_other(joint);
		if (other) {
			BasicPart* kid = BasicPart::of(zof_joint_part(other));
			if (kid != parent) {
				kid->transformBy(relative, BasicPart::of(body));
			}
		}
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
