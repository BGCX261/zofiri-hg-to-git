#include "zofiri.h"

extern "C" {

/**
 * Make the core unit centimeters for finer control.
 */
#define zof_bt_scale 100.0

struct zof_joint_struct {
	zof_type type;
	zof_str name;
	btTransform transform;
	zof_vec4 posLimits[2];
	zof_vec4 rotLimits[2];
};

/**
 * TODO Move this to zof.h sometime?
 */
typedef enum {
	// TODO Or just allow named groups at body root?
	// TODO When looking for joints, assume detached by default?
	// TODO That is, once linked, parts just become arbitrary.
	zof_part_kind_composite,
	zof_part_kind_primitive
} zof_part_kind;

/**
 * Thoughts on future part info to replace current.
 * TODO zof_material, ...
 */
struct zof_part_primitive {
	zof_shape shape;
	// TODO Material, ...
};

struct zof_part_struct {
	zof_type type;
	map<string,zof_joint> joints;
	zof_part_kind kind;
	zof_str name;
	union {
		// TODO Do we need a root body whether with kids or not?
		// TODO Or is the first kid the root?
		btRigidBody* body;
		vector<zof_part_struct*>* kids;
		zof_part_primitive partial;
	};
};

struct zof_shape_struct {
	zof_type type;
	btCollisionShape* shape;
};

struct zof_sim_struct {
	zof_type type;
	zof::Sim* sim;
};

zof_vec4 zof_bt3_to_vec4(btVector3 bt3, zof_num scale=1);
void zof_joint_close(zof_any part);
zof_type zof_joint_type(void);
void zof_part_close(zof_any part);
zof_type zof_part_type(void);
void zof_shape_close(zof_any part);
zof_type zof_shape_type(void);
void zof_sim_close(zof_any part);
zof_type zof_sim_type(void);
btVector3 zof_vec4_to_bt3(zof_vec4 vec, zof_num scale=1);
btVector4 zof_vec4_to_bt4(zof_vec4 vec, zof_num scale=1);

zof_vec4 zof_box_radii(zof_box box) {
	zof_part_struct* part_struct = (zof_part_struct*)box;
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

void zof_joint_close(zof_any part) {
	// TODO Free limits if set.
}

zof_str zof_joint_name(zof_joint joint) {
	return ((zof_joint_struct*)joint)->name;
}

zof_joint zof_joint_new(zof_str name, zof_vec4 pos, zof_vec4 rot) {
	// TODO Hrmm.
	zof_joint_struct* joint = (zof_joint_struct*)malloc(sizeof(zof_joint_struct));
	joint->type = zof_joint_type();
	joint->name = name;
	joint->transform.setOrigin(zof_vec4_to_bt3(pos, zof_bt_scale));
	joint->transform.setRotation(btQuaternion(zof_vec4_to_bt3(rot),btScalar(rot.vals[3])));
	// TODO Or would it be better just to store a partially filled btGeneric6DofConstraint?
	memset(joint->posLimits, 0, sizeof(joint->posLimits));
	memset(joint->rotLimits, 0, sizeof(joint->rotLimits));
	return (zof_joint)joint;
}

zof_type zof_joint_type(void) {
	static zof_type type = NULL;
	if (!type) {
		zof_type_info info;
		info.name = "zof_joint";
		info.close = zof_joint_close;
		type = zof_type_new(&info);
	}
	return type;
}

zof_bool zof_part_attach(zof_part part, zof_part kid) {
	// TODO ...
	// TODO False if no joints available?
	return zof_false;
}

zof_box zof_part_box(zof_part part) {
	return zof_part_shape_kind(part) == zof_shape_kind_box ? (zof_box)part : zof_null;
}

void zof_part_close(zof_any part) {
	zof_part_struct* part_struct = (zof_part_struct*)part;
	if (part_struct->kind == zof_part_kind_composite) {
		for (
			vector<zof_part_struct*>::iterator k = part_struct->kids->begin();
			k < part_struct->kids->end();
			k++
		) {
			zof_ref_free(*k);
		}
		delete part_struct->kids;
	} else {
		delete part_struct->body;
	}
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

zof_joint zof_part_joint_put(zof_part part, zof_joint joint) {
	zof_part_struct* part_struct = (zof_part_struct*)part;
	string name(zof_joint_name(joint));
	zof_joint old_joint = zof_null;
	map<string,zof_joint>::iterator old = part_struct->joints.find(name);
	if (old != part_struct->joints.end()) {
		// Old joint already there.
		old_joint = old->second;
	}
	cerr << name << " was at " << old_joint << endl;
	map<string,zof_joint>::value_type pair(name, joint);
	part_struct->joints.insert(old, pair);
	return old_joint;
}

zof_shape_kind zof_part_shape_kind(zof_part part) {
	zof_part_struct* part_struct = (zof_part_struct*)part;
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
	zof_part_struct* part = new zof_part_struct;
	part->type = zof_part_type();
	part->name = name;
	part->kind = zof_part_kind_primitive;
	// TODO This is mostly copied from Sim::createBody.
	// TODO We need to merge this sometime.
	// Actually setting the material will require recalculating mass props.
	zof::Material* material = zof::Material::defaultMaterial();
	btScalar volume = zof::Sim::calcVolume(((zof_shape_struct*)shape)->shape);
	btScalar mass(material->density * volume);
	btVector3 inertia(0,0,0);
	((zof_shape_struct*)shape)->shape->calculateLocalInertia(mass, inertia);
	zof::MotionState* motionState = new zof::MotionState();
	btTransform transform;
	transform.setIdentity();
	motionState->m_graphicsWorldTrans = transform;
	btRigidBody::btRigidBodyConstructionInfo bodyConstruct(mass, motionState, ((zof_shape_struct*)shape)->shape, inertia);
	part->body = new btRigidBody(bodyConstruct);
	zof::BodyInfo* info = new zof::BodyInfo;
	info->material = material;
	// Sim will need set when the part is added to the sim.
	// TODO When and how to copy parts? Obviously important need.
	info->sim = zof_null;
	part->body->setUserPointer(info);
	motionState->m_userPointer = part->body;
	return (zof_part)part;
}


zof_part zof_part_new_box(zof_str name, zof_vec4 radii) {
	return zof_part_new(name, zof_shape_new_box(radii));
}

zof_type zof_part_type(void) {
	static zof_type type = NULL;
	if (!type) {
		zof_type_info info;
		info.name = "zof_part";
		info.close = zof_part_close;
		type = zof_type_new(&info);
	}
	return type;
}

void zof_part_pos_put(zof_part part, zof_vec4 pos) {
	zof_part_struct* part_struct = (zof_part_struct*)part;
	if (part_struct->kind == zof_part_kind_primitive) {
		btVector3 bt = zof_vec4_to_bt3(pos, zof_bt_scale);
		part_struct->body->getWorldTransform().setOrigin(bt);
	}
}

void zof_shape_close(zof_any shape) {
	zof_shape_struct* shape_struct = (zof_shape_struct*)shape;
	delete shape_struct->shape;
}

zof_shape zof_shape_new_box(zof_vec4 radii) {
	zof_shape_struct* shape = (zof_shape_struct*)malloc(sizeof(zof_shape_struct));
	shape->type= zof_shape_type();
	btVector3 bt_radii = zof_vec4_to_bt3(radii, zof_bt_scale);
	btBoxShape* box = new btBoxShape(bt_radii);
	shape->shape = box;
	return (zof_shape)shape;
}

//zof_shape zof_shape_new_capsule(zof_num rad_xy, zof_num half_spread);

//zof_shape zof_shape_new_cylinder(zof_vec4 radii);

//zof_shape zof_shape_new_mesh(zof_mesh mesh);

zof_type zof_shape_type(void) {
	static zof_type type = NULL;
	if (!type) {
		zof_type_info info;
		info.name = "zof_shape";
		info.close = zof_shape_close;
		type = zof_type_new(&info);
	}
	return type;
}

void zof_sim_close(zof_any sim) {
	zof_sim_struct* sim_struct = (zof_sim_struct*)sim;
	delete sim_struct->sim;
}

void zof_sim_part_add(zof_sim sim, zof_part part) {
	zof_sim_struct* sim_struct = (zof_sim_struct*)sim;
	zof_part_struct* part_struct = (zof_part_struct*)part;
	if (part_struct->kind == zof_part_kind_primitive) {
		// TODO Add full graph whether composite or not?
		((zof::BodyInfo*)part_struct->body->getUserPointer())->sim = sim_struct->sim;
		sim_struct->sim->addBody(part_struct->body);
	}
}

zof_type zof_sim_type(void) {
	static zof_type type = NULL;
	if (!type) {
		zof_type_info info;
		info.name = "zof_sim";
		info.close = zof_sim_close;
		type = zof_type_new(&info);
	}
	return type;
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

namespace zof {

BodyInfo::BodyInfo():
		material(Material::defaultMaterial()),
		sceneNode(0),
		sim(0) {
	// Nothing more to do.
}

BodyInfo* BodyInfo::of(btCollisionObject* body) {
	return reinterpret_cast<BodyInfo*>(body->getUserPointer());
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
	BodyInfo* info = BodyInfo::of(body);
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

	// TODO Hack to make C-mod sim available.
	zof_sim_struct* sim_struct = (zof_sim_struct*)malloc(sizeof(zof_sim_struct));
	sim_struct->type = zof_sim_type();
	sim_struct->sim = this;
	csim = (zof_sim)sim_struct;
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
	BodyInfo* info = new BodyInfo;
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
	BodyInfo* info = new BodyInfo;
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
