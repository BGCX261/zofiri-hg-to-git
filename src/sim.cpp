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

zof_vec4 bt3ToVec4(const btVector3& bt3, zof_num scale=1);

/**
 * Changes any "_right" to "_left" or vice versa.
 */
void mirrorName(string* name);

void mirrorX(btTransform* transform);

btVector3 vec4ToBt3(zof_vec4 vec, zof_num scale=1);

btVector4 vec4ToBt4(zof_vec4 vec, zof_num scale=1);


struct Joint: Any {

	Joint(const string& name);

	/**
	 * Because this doesn't seem to be called automatically:
	 *   operator zof_joint() {
	 */
	zof_joint asC() {
		return reinterpret_cast<zof_joint>(this);
	}

	/**
	 * The kid's part will be transformed relative to this's part.
	 */
	void attach(Joint* kid);

	/**
	 * Doable if part, other, and remaining items already set.
	 */
	btGeneric6DofConstraint* createConstraint();

	/**
	 * Returns a mirror of this joint about the X axis, including any necessary changes to joint limits.
	 */
	Joint* mirror();

	static Joint* of(zof_joint joint) {
		return reinterpret_cast<Joint*>(joint);
	}

	string name;
	Part* part;
	Joint* other;
	btTransform transform;
	zof_vec4 posLimits[2];
	zof_vec4 rotLimits[2];

};

/**
 * For walking over chains with a callback.
 */
struct Walker: Any {
	virtual void handle(BasicPart* part) = 0;
	void walk(BasicPart* part) {
		// This grunt worker allows entirely hiding the recursive params.
		struct Grunt{
			Walker* walker;
			Grunt(Walker* w): walker(w) {}
			void walk(BasicPart* part, BasicPart* parent = 0) {
				walker->handle(part);
				map<string,Joint*>& joints = part->joints;
				for (map<string,Joint*>::iterator j = joints.begin(); j != joints.end(); j++) {
					Joint* joint = j->second;
					Joint* other = joint->other;
					if (other) {
						BasicPart* kid = other->part->basic();
						if (kid != parent) {
							walk(kid, part);
						}
					}
				}
			}
		} grunt(this);
		grunt.walk(part);
	}
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

zof_vec4 bt3ToVec4(const btVector3& bt3, zof_num scale) {
	zof_vec4 vec;
	for (zof_uint i = 0; i < 3; i++) {
		vec.vals[i] = scale * bt3.m_floats[i];
	}
	vec.vals[3] = 0;
	return vec;
}

void mirrorName(string* name) {
	// TODO Should smartify this. Oh for regexps.
	string left("_left");
	string right("_right");
	string::size_type pos = name->rfind(left);
	if (pos != string::npos) {
		name->replace(pos, left.size(), right);
	} else {
		// No left found, so try right.
		pos = name->rfind(right);
		if (pos != string::npos) {
			name->replace(pos, right.size(), left);
		}
	}
}

void mirrorX(btTransform* transform) {
	cerr << "Mirroring transform from " << *transform;
	// TODO What to do with rotations?
	// TODO No good: *transform *= btTransform(btMatrix3x3(-1,0,0, 0,1,0, 0,0,1), btVector3(0,0,0));
	// TODO Is there a better way than this?
	//transform->setRotation(transform->getRotation() *= btQuaternion(btVector3(0,0,1),zof_pi));
	transform->getOrigin().setX(-transform->getOrigin().getX());
	cerr << " to " << *transform << endl;
}

btVector3 vec4ToBt3(zof_vec4 vec, zof_num scale) {
	btVector3 bt;
	for (zof_uint i = 0; i < 3; i++) {
		bt.m_floats[i] = btScalar(scale * vec.vals[i]);
	}
	bt.m_floats[3] = btScalar(0);
	return bt;
}

btVector4 vec4ToBt4(zof_vec4 vec, zof_num scale) {
	btVector4 bt;
	for (zof_uint i = 0; i < 4; i++) {
		bt.m_floats[i] = btScalar(scale * vec.vals[i]);
	}
	return bt;
}

}


using namespace zof;

extern "C" {

zof_vec4 zof_box_radii(zof_box box) {
	btBoxShape* shape = reinterpret_cast<btBoxShape*>(BasicPart::of(box)->body->getCollisionShape());
	btVector3 radii = shape->getHalfExtentsWithMargin();
	return bt3ToVec4(radii, 1/zof_bt_scale);
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
	btVector3 bt_axis = vec4ToBt3(axis);
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
    return bt3ToVec4(origin, 1/zof_bt_scale);
}

zof_export zof_num zof_capsule_radius(zof_capsule capsule) {
	btCapsuleShape* shape = reinterpret_cast<btCapsuleShape*>(
		BasicPart::of(capsule)->body->getCollisionShape()
	);
	return shape->getRadius() / zof_bt_scale;
}

void zof_joint_attach(zof_joint joint, zof_joint kid) {
	// TODO Return anything?
	Joint::of(joint)->attach(Joint::of(kid));
}

zof_str zof_joint_name(zof_joint joint) {
	return zof_str(((Joint*)joint)->name.c_str());
}

zof_joint zof_joint_new(zof_str name, zof_vec4 pos, zof_vec4 rot) {
	Joint* joint = new Joint(name);
	joint->transform.setRotation(btQuaternion(vec4ToBt3(rot),btScalar(rot.vals[3])));
	joint->transform.setOrigin(vec4ToBt3(pos,zof_bt_scale));
	return joint->asC();
}

zof_joint zof_joint_other(zof_joint joint) {
	return Joint::of(joint)->other->asC();
}

zof_part zof_joint_part(zof_joint joint) {
	return ((Joint*)joint)->part->asC();
}

zof_material zof_material_new(zof_color color, zof_num density) {
	Material* material = new Material(color);
	material->density = btScalar(density);
	return reinterpret_cast<zof_material>(material);
}

zof_bool zof_part_attach(zof_part part, zof_part kid) {
	// TODO Some casting operator for general use?
	return Part::of(part)->attach(Part::of(kid)) ? zof_true : zof_false;
}

zof_box zof_part_box(zof_part part) {
	return zof_part_part_kind(part) == zof_part_kind_box ? (zof_box)part : zof_null;
}

zof_capsule zof_part_capsule(zof_part part) {
	return zof_part_part_kind(part) == zof_part_kind_capsule ? (zof_capsule)part : zof_null;
}

zof_vec4 zof_part_end_pos(zof_part part, zof_vec4 ratios) {
	// TODO Separate bounds function? What about non-centered origin?
	zof_vec4 radii;
	switch (zof_part_part_kind(part)) {
	case zof_part_kind_box:
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
	return Part::of(part)->joint(name)->asC();
}

zof_joint zof_part_joint_put(zof_part part, zof_joint joint) {
	return Part::of(part)->jointPut(Joint::of(joint))->asC();
}

void zof_part_material_put(zof_part part, zof_material material) {
	Part::of(part)->setMaterial(reinterpret_cast<Material*>(material));
}

zof_part zof_part_mirror(zof_part part) {
	return Part::of(part)->mirror()->asC();
}

zof_str zof_part_name(zof_part part) {
	return const_cast<zof_str>(Part::of(part)->name.c_str());
}

zof_export void zof_part_name_put(zof_part part, zof_str name) {
	Part::of(part)->name = name;
}

zof_part_kind zof_part_part_kind(zof_part part) {
	Part* abstractPart = Part::of(part);
	GroupPart* group = dynamic_cast<GroupPart*>(abstractPart);
	if (group) {
		return zof_part_kind_group;
	}
	BasicPart* basic = dynamic_cast<BasicPart*>(abstractPart);
	if (!basic) {
		// What is this thing anyway?
		return zof_part_kind_error;
	}
	switch(basic->body->getCollisionShape()->getShapeType()) {
	case BOX_SHAPE_PROXYTYPE:
		return zof_part_kind_box;
	case CAPSULE_SHAPE_PROXYTYPE:
		return zof_part_kind_capsule;
	case CYLINDER_SHAPE_PROXYTYPE:
		return zof_part_kind_cylinder;
	default:
		// Unsupported shape type.
		return zof_part_kind_error;
	}
}

zof_part zof_part_new_box(zof_str name, zof_vec4 radii) {
	btVector3 bt_radii = vec4ToBt3(radii, zof_bt_scale);
	BasicPart* part = new BasicPart(name, new btBoxShape(bt_radii));
	return part->asC();
}

zof_part zof_part_new_capsule(zof_str name, zof_num radius, zof_num half_spread) {
	BasicPart* part = new BasicPart(name, new btCapsuleShape(radius*zof_bt_scale,2*half_spread*zof_bt_scale));
	return part->asC();
}

zof_export zof_part zof_part_new_cylinder(zof_str name, zof_vec4 radii) {
	btVector3 bt_radii = vec4ToBt3(radii, zof_bt_scale);
	BasicPart* part = new BasicPart(name, new btCylinderShape(bt_radii));
	return part->asC();
}

zof_part zof_part_new_group(zof_str name, zof_part root) {
	return (new GroupPart(name, Part::of(root)))->asC();
}

void zof_part_pos_add(zof_part part, zof_vec4 pos) {
	// TODO
}

void zof_part_pos_put(zof_part part, zof_vec4 pos) {
	Part::of(part)->setPos(vec4ToBt3(pos, zof_bt_scale));
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
		for (map<string,Joint*>::iterator j = part_struct->joints.begin(); j != part_struct->joints.end(); j++) {
			Joint* joint = j->second;
			Joint* other = joint->other;
			if (other) {
				Part* kid = other->part;
				//cerr << "Found attached " << zof_part_name(kid) << endl;
				zof_sim_part_add(sim, kid->asC());
				btGeneric6DofConstraint* constraint = joint->createConstraint();
				// TODO Max the mins of joint and other, and min the maxes.
				constraint->setAngularLowerLimit(vec4ToBt3(joint->rotLimits[0]));
				constraint->setAngularUpperLimit(vec4ToBt3(joint->rotLimits[1]));
				// TODO Pos limits.
				simPriv->addConstraint(constraint);
			}
		}
	}
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
	btScalar volume = Sim::calcVolume(shape);
	// TODO Unify mass props calculation with setMaterial?
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

Part* BasicPart::mirror() {
	// TODO Copy the shape? Well, especially for meshes (for mirroring). Maybe others if mutable someday.
	string otherName(name);
	mirrorName(&otherName);
	Part* mirrorPart = new BasicPart(otherName, body->getCollisionShape());
	mirrorPart->setMaterial(material, false);
	//cerr << "Created mirror part " << mirrorPart->name << endl;
	Joint* attachedJoint = 0;
	Joint* otherJoint = 0;
	bool multi = false;
	for (map<string,Joint*>::iterator j = joints.begin(); j != joints.end(); j++) {
		Joint* joint = j->second;
		Joint* mirrored = joint->mirror();
		mirrorPart->jointPut(mirrored);
		if (joint->other) {
			// It's attached. We want to attach to a mirror of this.
			if (attachedJoint) {
				multi = true;
			} else {
				attachedJoint = mirrored;
				otherJoint = joint->other;
			}
		}
	}
	if (attachedJoint && !multi) {
		Joint* mirroredOther = otherJoint->mirror();
		otherJoint->part->jointPut(mirroredOther);
		mirroredOther->attach(attachedJoint);
	}
	return mirrorPart;
}

Joint::Joint(const string& name) {
	this->name = name;
	part = 0;
	other = 0;
	transform.setIdentity();
	// TODO Or would it be better just to store a partially filled btGeneric6DofConstraint?
	memset(posLimits, 0, sizeof(posLimits));
	memset(rotLimits, 0, sizeof(rotLimits));
}

void Joint::attach(Joint* kid) {
	// TODO What if already attached??
	btTransform transform = part->getTransform();
	//cerr << "part " << part->name << " at " << transform << endl;
	btTransform relTransform = this->transform * kid->transform.inverse();
	//cerr << "relTransform of " << relTransform << endl;
	transform *= relTransform;
	//cerr << "kid " << kid->part->name << " at " << transform << endl;
	kid->part->setTransform(transform);
	// Now actually attach joints.
	other = kid;
	kid->other = this;
}

btGeneric6DofConstraint* Joint::createConstraint() {
	btGeneric6DofConstraint* constraint = new btGeneric6DofConstraint(*part->body, *other->part->body, transform, other->transform, false);
	// TODO Limits, etc.
	return constraint;
}

Joint* Joint::mirror() {
	Joint* mirrored = new Joint(name);
	mirrorName(&mirrored->name);
	mirrored->transform = transform;
	mirrorX(&mirrored->transform);
	//cerr << "Mirroring joint from " << name << " at " << transform << " to " << mirrored->name << " at " << mirrored->transform << endl;
	for (int L = 0; L <= 1; L++) {
		// TODO Mirror the limits!?! Think through this.
		mirrored->posLimits[L] = posLimits[L];
		mirrored->rotLimits[L] = rotLimits[L];
	}
	return mirrored;
}

GroupPart::GroupPart(const string& name, Part* root): Part(name) {
	body = root->basic()->body;
	init(root->basic());
}

void GroupPart::init(BasicPart* part, BasicPart* parent) {
	// Recurse around, finding detached joints.
	//cerr << "init " << name << " at " << part->name << endl;
	map<string,Joint*>& joints = part->joints;
	for (map<string,Joint*>::iterator j = joints.begin(); j != joints.end(); j++) {
		Joint* joint = j->second;
		Joint* other = joint->other;
		if (other) {
			BasicPart* kid = other->part->basic();
			if (kid != parent) {
				init(kid, part);
			}
		} else {
			// Add detached joints to the group for nice access.
			// This assumes (somewhat fairly) that they'll have unique names.
			//cerr << "joint " << name << " to " << zof_joint_name(joint) << endl;
			this->joints[joint->name] = joint;
		}
	}
	// Go up the group chain until none set, then set to this.
	// Note that this is a case where bottom-up is less efficient.
	// However, part hierarchies don't seem likely to be deep, so probably okay.
	// TODO Do I really need to dynamic_cast to guarantee equal pointer values???
	// TODO The cerr log of pointer below indicates not, but I haven't tested all platforms.
	Part* partInGroup = part;
	Part* thisAsPart = dynamic_cast<Part*>(this);
	// cerr << this << " vs. " << thisAsPart << endl;
	while (partInGroup != thisAsPart) {
		if (!partInGroup->group) {
			partInGroup->group = this;
			//cerr << "group " << name << " for " << partInGroup->name << endl;
		}
		partInGroup = partInGroup->group;
	}
}

Part* GroupPart::mirror() {
	// TODO !!
	return 0;
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

zof_part Part::asC() {
	return reinterpret_cast<zof_part>(this);
}

bool Part::attach(Part* kid) {
	//cerr << "attach to " << name << " kid " << kid->name << endl;
	Joint* kidJoint = kid->joint(name);
	if (kidJoint) {
		Joint* partJoint = joint(kid->name);
		if (partJoint) {
			// TODO What if previously attached to other parts?
			partJoint->attach(kidJoint);
			return true;
		}
	}
	return false;
}

BasicPart* Part::basic() {
	return BasicPart::of(body);
}

bool Part::inGroup(GroupPart* group) {
	GroupPart* ownGroup = this->group;
	while (ownGroup) {
		if (ownGroup == group) {
			return true;
		}
		ownGroup = ownGroup->group;
	}
	return false;
}

Joint* Part::joint(const string& name) {
	map<string,Joint*>::iterator j = joints.find(name);
	return j == joints.end() ? 0 : j->second;
}

Joint* Part::jointPut(Joint* joint) {
	// TODO Or just disallow all non-BasicParts? Here we add to the basic.
	BasicPart* part = basic();
	Joint* oldJoint = 0;
	map<string,Joint*>::iterator old = part->joints.find(joint->name);
	if (old != part->joints.end()) {
		// Old joint already there.
		oldJoint = old->second;
		oldJoint->part = 0;
	}
	//cerr << "Creating a pair for joint " << joint->name << endl;
	map<string,Joint*>::value_type pair(joint->name, joint);
	part->joints.insert(old, pair);
	joint->part = part;
	return oldJoint;
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

void Part::setMaterial(Material* material, bool flood) {
	struct MaterialWalker: Walker {
		Material *material, *original;
		MaterialWalker(Material* m, Material* o): material(m), original(o) {}
		void handle(BasicPart* part) {
			if (part->material == original) {
				update(part);
			}
		}
		void update(BasicPart* part) {
			// TODO How and when to free existing? <-- Assume held in DB elsewhere???
			part->material = material;
			// Recalculate and setMassProps.
			btScalar volume = Sim::calcVolume(part->body->getCollisionShape());
			btScalar mass(material->density * volume);
			btVector3 inertia(0,0,0);
			part->body->getCollisionShape()->calculateLocalInertia(mass, inertia);
			part->body->setMassProps(mass, inertia);
		}
	} walker(material, basic()->material);
	if (flood) {
		walker.walk(basic());
	} else {
		walker.update(basic());
	}
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
	//cerr << "Moving " << name << " from " << getTransform();
	body->setWorldTransform(relative * body->getWorldTransform());
	map<string,Joint*>& joints = basic()->joints;
	//cerr << " to " << getTransform() << " by " << relative << endl;
	for (map<string,Joint*>::iterator j = joints.begin(); j != joints.end(); j++) {
		Joint* joint = j->second;
		Joint* other = joint->other;
		if (other) {
			BasicPart* kid = other->part->basic();
			if (kid != parent) {
				kid->transformBy(relative, basic());
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
