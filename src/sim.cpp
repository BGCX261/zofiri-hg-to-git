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
#define zofBtScale 100.0

namespace zof {

zofVec4 bt3ToVec4(const btVector3& bt3, zofNum scale=1);

/**
 * Changes any "Right" to "Left" or vice versa.
 */
void mirrorName(string* name);

void mirrorX(btTransform* transform);

/**
 * Replaces the last occurrence of oldSub with newSub.
 */
bool replaceLast(string* text, const string& oldSub, const string& newSub);

btVector3 vec4ToBt3(zofVec4 vec, zofNum scale=1);

btVector4 vec4ToBt4(zofVec4 vec, zofNum scale=1);


struct Joint: Any {

	Joint(const string& name);

	/**
	 * Because this doesn't seem to be called automatically:
	 *   operator zof_joint() {
	 */
	zofJoint asC() {
		return reinterpret_cast<zofJoint>(this);
	}

	/**
	 * The kid's part will be transformed relative to this's part.
	 */
	void attach(Joint* kid);

	/**
	 * It doesn't copy part or other over, just the joint info.
	 */
	Joint* copy();

	/**
	 * Doable if part, other, and remaining items already set.
	 */
	btGeneric6DofConstraint* createConstraint();

	void limitsRotPut(const zofRad3& min, const zofRad3& max);

	/**
	 * Returns a mirror of this joint about the X axis, including any necessary changes to joint limits.
	 */
	Joint* mirror();

	static Joint* of(zofJoint joint) {
		return reinterpret_cast<Joint*>(joint);
	}

	void velPut(zofNum vel);

	btGeneric6DofConstraint* constraint;
	string name;
	Part* part;
	Joint* other;
	zofExtents3 posLimits;
	zofExtents3 rotLimits;
	btTransform transform;

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

zofVec4 bt3ToVec4(const btVector3& bt3, zofNum scale) {
	zofVec4 vec;
	for (zofUint i = 0; i < 3; i++) {
		vec.vals[i] = scale * bt3.m_floats[i];
	}
	vec.vals[3] = 0;
	return vec;
}

void mirrorName(string* name) {
	// TODO Should smartify this. Oh for regexps. Consider bundling V8 (for that and more).
	if (!replaceLast(name, "Left", "Right")) {
		replaceLast(name, "Right", "Left");
	}
}

void mirrorX(btTransform* transform) {
	//cerr << "Mirroring transform from " << *transform;
	// TODO What to do with rotations?
	// TODO No good: *transform *= btTransform(btMatrix3x3(-1,0,0, 0,1,0, 0,0,1), btVector3(0,0,0));
	// TODO Is there a better way than this?
	transform->setRotation(transform->getRotation() *= btQuaternion(btVector3(0,0,1),zofPi));
	transform->getOrigin().setX(-transform->getOrigin().getX());
	//cerr << " to " << *transform << endl;
}

bool replaceLast(string* text, const string& oldSub, const string& newSub) {
	string::size_type pos = text->rfind(oldSub);
	if (pos == string::npos) {
		return false;
	}
	text->replace(pos, oldSub.size(), newSub);
	return true;
}

btVector3 vec4ToBt3(zofVec4 vec, zofNum scale) {
	btVector3 bt;
	for (zofUint i = 0; i < 3; i++) {
		bt.m_floats[i] = btScalar(scale * vec.vals[i]);
	}
	bt.m_floats[3] = btScalar(0);
	return bt;
}

btVector4 vec4ToBt4(zofVec4 vec, zofNum scale) {
	btVector4 bt;
	for (zofUint i = 0; i < 4; i++) {
		bt.m_floats[i] = btScalar(scale * vec.vals[i]);
	}
	return bt;
}

}


using namespace zof;

extern "C" {

zofVec4 zofCapsuleEndPos(zofCapsule capsule, zofNum radius_ratio) {
	return zofCapsuleEndPosEx(capsule, radius_ratio, zofXyz(0,1,0), 1);
}

zofVec4 zofCapsuleEndPosEx(
	zofCapsule capsule,
	zofNum radius_ratio,
	zofVec4 axis,
	zofNum half_spread_ratio
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
    return bt3ToVec4(origin, 1/zofBtScale);
}

zofExport zofNum zofCapsuleRadius(zofCapsule capsule) {
	btCapsuleShape* shape = reinterpret_cast<btCapsuleShape*>(
		BasicPart::of(capsule)->body->getCollisionShape()
	);
	return shape->getRadius() / zofBtScale;
}

void zofJointAttach(zofJoint joint, zofJoint kid) {
	// TODO Return anything?
	Joint::of(joint)->attach(Joint::of(kid));
}

void zofJointLimitsRotPut(zofJoint joint, zofRad3 min, zofRad3 max) {
	Joint::of(joint)->limitsRotPut(min, max);
}

zofString zofJointName(zofJoint joint) {
	return zofString(Joint::of(joint)->name.c_str());
}

zofJoint zofJointNew(zofString name, zofVec4 pos) {
	return zofJointNewEx(name, pos, zofXyzw(0,1,0,0));
}

zofJoint zofJointNewEx(zofString name, zofVec4 pos, zofVec4 rot) {
	Joint* joint = new Joint(name);
	joint->transform.setRotation(btQuaternion(vec4ToBt3(rot),btScalar(rot.vals[3])));
	joint->transform.setOrigin(vec4ToBt3(pos,zofBtScale));
	return joint->asC();
}

zofJoint zofJointOther(zofJoint joint) {
	return Joint::of(joint)->other->asC();
}

zofPart zofJointPart(zofJoint joint) {
	return Joint::of(joint)->part->asC();
}

void zofJointVelPut(zofJoint joint, zofNum vel) {
	Joint::of(joint)->velPut(vel);
}

zofMaterial zofMaterialNew(zofColor color, zofNum density) {
	Material* material = new Material(color);
	material->density = btScalar(density);
	return material->asC();
}

zofBool zofPartAttach(zofPart part, zofPart kid) {
	// TODO Some casting operator for general use?
	return Part::of(part)->attach(Part::of(kid)) ? zofTrue : zofFalse;
}

zofBox zofPartBox(zofPart part) {
	return zofPartPartKind(part) == zofPartKindBox ? (zofBox)part : zofNull;
}

zofCapsule zofPartCapsule(zofPart part) {
	return zofPartPartKind(part) == zofPartKindCapsule ? (zofCapsule)part : zofNull;
}

zofPart zofPartCopyTo(zofPart part, zofM3 pos, zofString oldSub, zofString newSub) {
	return Part::of(part)->copyTo(vec4ToBt3(pos, zofBtScale), string(oldSub), string(newSub))->asC();
}

zofVec4 zofPartEndPos(zofPart part, zofVec4 ratios) {
	zofVec4 radii = zofPartRadii(part);
	for (int i = 0; i < 4; i++) {
		radii.vals[i] *= ratios.vals[i];
	}
	return radii;
}

zofExport zofExtentsM3 zofPartExtents(zofPart part) {
	zofExtentsM3 extents;
	btVector3 min, max;
	Part::of(part)->extents(&min, &max);
	extents.min = bt3ToVec4(min, 1/zofBtScale);
	extents.max = bt3ToVec4(max, 1/zofBtScale);
	return extents;
}

zofJoint zofPartJoint(zofPart part, zofString name) {
	return Part::of(part)->joint(name)->asC();
}

zofJoint zofPartJointPut(zofPart part, zofJoint joint) {
	return Part::of(part)->jointPut(Joint::of(joint))->asC();
}

zofMaterial zofPartMaterial(zofPart part) {
	return BasicPart::of(part)->material->asC();
}

void zofPartMaterialPut(zofPart part, zofMaterial material) {
	Part::of(part)->setMaterial(Material::of(material));
}

zofPart zofPartMirror(zofPart part) {
	return Part::of(part)->mirror()->asC();
}

zofString zofPartName(zofPart part) {
	return const_cast<zofString>(Part::of(part)->name.c_str());
}

zofExport void zofPartNamePut(zofPart part, zofString name) {
	Part::of(part)->name = name;
}

zofPartKind zofPartPartKind(zofPart part) {
	Part* abstractPart = Part::of(part);
	GroupPart* group = dynamic_cast<GroupPart*>(abstractPart);
	if (group) {
		return zofPartKindGroup;
	}
	BasicPart* basic = dynamic_cast<BasicPart*>(abstractPart);
	if (!basic) {
		// What is this thing anyway?
		return zofPartKindError;
	}
	switch(basic->body->getCollisionShape()->getShapeType()) {
	case BOX_SHAPE_PROXYTYPE:
		return zofPartKindBox;
	case CAPSULE_SHAPE_PROXYTYPE:
		return zofPartKindCapsule;
	case CYLINDER_SHAPE_PROXYTYPE:
		return zofPartKindCylinder;
	default:
		// Unsupported shape type.
		return zofPartKindError;
	}
}

zofPart zofPartNewBox(zofString name, zofVec4 radii) {
	btVector3 bt_radii = vec4ToBt3(radii, zofBtScale);
	BasicPart* part = new BasicPart(name, new btBoxShape(bt_radii));
	return part->asC();
}

zofPart zofPartNewCapsule(zofString name, zofNum radius, zofNum half_spread) {
	BasicPart* part = new BasicPart(name, new btCapsuleShape(radius*zofBtScale,2*half_spread*zofBtScale));
	return part->asC();
}

zofPart zofPartNewCylinder(zofString name, zofVec4 radii) {
	btVector3 bt_radii = vec4ToBt3(radii, zofBtScale);
	BasicPart* part = new BasicPart(name, new btCylinderShape(bt_radii));
	return part->asC();
}

zofPart zofPartNewGroup(zofString name, zofPart root) {
	return (new GroupPart(name, Part::of(root)))->asC();
}

zofVec4 zofPartPos(zofPart part) {
	return bt3ToVec4(Part::of(part)->getTransform().getOrigin(), 1/zofBtScale);
}

void zofPartPosAdd(zofPart part, zofVec4 pos) {
	// TODO
}

void zofPartPosPut(zofPart part, zofVec4 pos) {
	Part::of(part)->setPos(vec4ToBt3(pos, zofBtScale));
}

zofVec4 zofPartRadii(zofPart part) {
	// TODO What about non-centered origins?
	zofVec4 radii;
	switch (zofPartPartKind(part)) {
	case zofPartKindBox:
	case zofPartKindCylinder: {
		btVector3 min, max;
		Part::of(part)->extents(&min, &max);
		radii = bt3ToVec4(max, 1/zofBtScale);
		break;
	}
	default:
		// TODO Not easy to indicate error here!
		memset(&radii, 0, sizeof(radii));
		break;
	}
	return radii;
}

void zofPartRotAdd(zofPart part, zofVec4 rot) {
	// TODO
}

void zofPartRotPut(zofPart part, zofVec4 rot) {
	// TODO
}

void zofSimPartAdd(zofSim sim, zofPart part) {
	Sim* simPriv = reinterpret_cast<Sim*>(sim);
	BasicPart* partStruct = BasicPart::of(part);
	// Add full graph whether composite or not.
	// Avoid infinite recursion by checking sim.
	// TODO If in another sim, move to this one?
	if (!partStruct->sim) {
		partStruct->sim = simPriv;
		simPriv->addBody(partStruct->body);
		//cerr << partStruct->name << " #joints: " << partStruct->joints.size() << endl;
		for (map<string,Joint*>::iterator j = partStruct->joints.begin(); j != partStruct->joints.end(); j++) {
			Joint* joint = j->second;
			Joint* other = joint->other;
			if (other) {
				Part* kid = other->part;
				if (!kid->basic()->sim) {
					//cerr << "Found attached " << kid->name << endl;
					zofSimPartAdd(sim, kid->asC());
					btGeneric6DofConstraint* constraint = joint->createConstraint();
					simPriv->addConstraint(constraint);
				}
			}
		}
	}
}

void zofSimUpdaterAdd(zofSim sim, void (*updater)(zofSim,zofAny), zofAny data) {
	struct CallbackUpdater: Updater {
		void (*updater)(zofSim,zofAny); zofAny data;
		CallbackUpdater(void(*u)(zofSim,zofAny), zofAny d): updater(u), data(d) {}
		virtual void update(Sim* sim) {
			updater(sim->asC(), data);
		}
	};
	Sim::of(sim)->updaters.push_back(new CallbackUpdater(updater, data));
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
	sim = zofNull;
	body->setUserPointer(this);
	motionState->m_userPointer = body;
}

BasicPart::~BasicPart() {
	// TODO Remove it from the sim first, along with constraints?
	// TODO Remove attached parts?
	delete body;
}

Part* BasicPart::copyTo(const btVector3& pos, const string& oldSub, const string& newSub) {
	// TODO Merge with mirror (and others later)?
	// TODO Copy the shape? Well, especially for meshes (for mirroring). Maybe others if mutable someday.
	string otherName(name);
	replaceLast(&otherName, oldSub, newSub);
	Part* mirrorPart = new BasicPart(otherName, body->getCollisionShape());
	mirrorPart->setMaterial(material, false);
	//cerr << "Created mirror part " << mirrorPart->name << endl;
	Joint* attachedJoint = 0;
	Joint* otherJoint = 0;
	bool multi = false;
	for (map<string,Joint*>::iterator j = joints.begin(); j != joints.end(); j++) {
		Joint* joint = j->second;
		Joint* copied = joint->copy();
		mirrorPart->jointPut(copied);
		if (joint->other) {
			// It's attached. We want to attach to a mirror of this.
			if (attachedJoint) {
				multi = true;
			} else {
				attachedJoint = copied;
				otherJoint = joint->other;
			}
		}
	}
	if (attachedJoint && !multi) {
		Joint* copiedOther = otherJoint->copy();
		replaceLast(&copiedOther->name, oldSub, newSub);
		copiedOther->transform.setOrigin(pos);
		otherJoint->part->jointPut(copiedOther);
		copiedOther->attach(attachedJoint);
	}
	return mirrorPart;
}

void BasicPart::extents(btVector3* min, btVector3* max) {
	switch(body->getCollisionShape()->getShapeType()) {
	case BOX_SHAPE_PROXYTYPE: {
		btBoxShape* shape = reinterpret_cast<btBoxShape*>(body->getCollisionShape());
		*max = shape->getHalfExtentsWithMargin();
		break;
	}
	case CYLINDER_SHAPE_PROXYTYPE: {
		btCylinderShape* shape = reinterpret_cast<btCylinderShape*>(body->getCollisionShape());
		*max = shape->getHalfExtentsWithMargin();
		break;
	}
	case CAPSULE_SHAPE_PROXYTYPE:
	default: {
		// TODO Indicate error.
		min->setZero();
		max->setZero();
		break;
	}
	}
	*min = -*max;
}

void BasicPart::init() {
	material = Material::defaultMaterial();
	sceneNode = 0;
	sim = 0;
}

BasicPart* BasicPart::of(btCollisionObject* body) {
	return reinterpret_cast<BasicPart*>(body->getUserPointer());
}

BasicPart* BasicPart::of(zofBox box) {
	return reinterpret_cast<BasicPart*>(box);
}

BasicPart* BasicPart::of(zofCapsule capsule) {
	return reinterpret_cast<BasicPart*>(capsule);
}

BasicPart* BasicPart::of(zofPart part) {
	return Part::of(part)->basic();
}

Part* BasicPart::mirror() {
	// TODO Merge with copyTo (and others later)?
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
		Joint* copied = joint->copy();
		mirrorPart->jointPut(copied);
		if (joint->other) {
			// It's attached. We want to attach to a mirror of this.
			if (attachedJoint) {
				multi = true;
			} else {
				attachedJoint = copied;
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
	constraint = 0;
	other = 0;
	part = 0;
	transform.setIdentity();
	// TODO Or would it be better just to store a partially filled btGeneric6DofConstraint?
	memset(&posLimits, 0, sizeof(posLimits));
	memset(&rotLimits, 0, sizeof(rotLimits));
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

Joint* Joint::copy() {
	Joint* joint = new Joint(name);
	joint->transform = transform;
	joint->posLimits = posLimits;
	joint->rotLimits = rotLimits;
	return joint;
}

btGeneric6DofConstraint* Joint::createConstraint() {
	struct Limits {
		static void constraintLimits(btVector3* minOut, btVector3* maxOut, const zofExtents3& a, const zofExtents3& b) {
			// TODO Normalize angles first?
			for (int i = 0; i < 3; i++) {
				if (abnormal(a.min.vals[i]) || abnormal(a.max.vals[i]) || abnormal(b.min.vals[i]) || abnormal(b.max.vals[i])) {
					// Unconstrained.
					//cerr << "Abnormal axis " << i << endl;
					minOut->m_floats[i] = 1;
					maxOut->m_floats[i] = -1;
				} else {
					minOut->m_floats[i] = btScalar(btMin(a.min.vals[i], b.min.vals[i]));
					maxOut->m_floats[i] = btScalar(btMax(a.max.vals[i], b.max.vals[i]));
				}
				//minOut->m_floats[i] = 0;
				//maxOut->m_floats[i] = 0;
			}
		}
		static bool abnormal(zofNum num) {
			return num != num || num == zofInf || num == -zofInf;
		}
	};
	if (!(part && other && other->part)) {
		return 0;
	}
	//cerr << "Creating constraint for joint from " << part->name << " to " << name << endl;
	btGeneric6DofConstraint* constraint = new btGeneric6DofConstraint(*part->body, *other->part->body, transform, other->transform, false);
	// TODO Unconstrained to Bullet means lower > upper.
	// TODO Consider other, and make the limits be the extreme of the two?
	btVector3 min, max;
	Limits::constraintLimits(&min, &max, rotLimits, other->rotLimits);
	constraint->setAngularLowerLimit(min);
	constraint->setAngularUpperLimit(max);
	Limits::constraintLimits(&min, &max, posLimits, other->posLimits);
	constraint->setLinearLowerLimit(min*zofBtScale);
	constraint->setLinearUpperLimit(max*zofBtScale);
	for (int i = 0; i < 3; i++) {
		if (constraint->getRotationalLimitMotor(i)->m_loLimit != constraint->getRotationalLimitMotor(i)->m_hiLimit) {
			//cerr << "Joint to " << name << " rot axis " << i << " can move." << endl;
			constraint->getRotationalLimitMotor(i)->m_enableMotor = true;
			// Sample code just to see any effect. So far none.
			//constraint->getRotationalLimitMotor(i)->m_maxMotorForce = 500;
			//constraint->getRotationalLimitMotor(i)->m_targetVelocity = 500;
		}
		if (constraint->getTranslationalLimitMotor()->m_lowerLimit.m_floats[i] != constraint->getTranslationalLimitMotor()->m_upperLimit.m_floats[i]) {
			//cerr << "Joint to " << name << " pos axis " << i << " can move." << endl;
			constraint->getTranslationalLimitMotor()->m_enableMotor[i] = true;
		}
	}
	//	cerr << "Joint to " << name
	//		<< " angular lower " << btVector3(constraint->getRotationalLimitMotor(0)->m_loLimit, constraint->getRotationalLimitMotor(1)->m_loLimit, constraint->getRotationalLimitMotor(2)->m_loLimit)
	//		<< " upper" << btVector3(constraint->getRotationalLimitMotor(0)->m_hiLimit, constraint->getRotationalLimitMotor(1)->m_hiLimit, constraint->getRotationalLimitMotor(2)->m_hiLimit)
	//		<< endl;
	this->constraint = constraint;
	other->constraint = constraint;
	return constraint;
}

void Joint::limitsRotPut(const zofRad3& min, const zofRad3& max) {
	rotLimits.min = min;
	rotLimits.max = max;
}

Joint* Joint::mirror() {
	Joint* mirrored = copy();
	mirrorName(&mirrored->name);
	mirrorX(&mirrored->transform);
	//cerr << "Mirroring joint from " << name << " at " << transform << " to " << mirrored->name << " at " << mirrored->transform << endl;
	// TODO Mirror the limits!?! Think through this.
	return mirrored;
}

void Joint::velPut(zofNum vel) {
	if (!constraint) {
		return;
	}
	int index = -1;
	bool multi = false;
	bool rot = true;
	for (int i = 0; i < 3; i++) {
		if (rotLimits.min.vals[i] != rotLimits.max.vals[i]) {
			if (index >= 0) {
				multi = true;
				break;
			}
			index = i;
		}
		if (posLimits.min.vals[i] != posLimits.max.vals[i]) {
			if (index >= 0) {
				multi = true;
				break;
			}
			index = i;
			rot = false;
		}
	}
	if (multi) {
		index = -1;
	}
	if (index >= 0) {
		//cerr << "Setting target vel for " << name << " to " << vel << endl;
		if (rot) {
			constraint->getRotationalLimitMotor(index)->m_targetVelocity = btScalar(vel);
		} else {
			constraint->getTranslationalLimitMotor()->m_targetVelocity.m_floats[index] = btScalar(vel);
		}
	}
}

GroupPart::GroupPart(const string& name, Part* root): Part(name) {
	body = root->basic()->body;
	init(root->basic());
}

Part* GroupPart::copyTo(const btVector3& pos, const string& oldSub, const string& newSub) {
	// TODO !!
	return 0;
}

void GroupPart::extents(btVector3* min, btVector3* max) {
	min->setZero();
	max->setZero();
	struct ExtentsWalker: Walker {
		GroupPart* group; btVector3* min; btVector3* max;
		ExtentsWalker(GroupPart* group_, btVector3* min_, btVector3* max_): group(group_), min(min_), max(max_) {}
		void handle(BasicPart* part) {
			//cerr << "Walking at " << part->name << endl;
			if (part->inGroup(group)) {
				btVector3 partMin, partMax;
				part->extents(&partMin, &partMax);
				const btTransform& transform = part->getTransform();
				partMin = transform(partMin);
				partMax = transform(partMax);
				//cerr << "  Min " << partMin << " and max " << partMax << endl;
				// TODO Batch forms of min and max would sure be nice.
				min->setX(btMin(min->getX(),partMin.getX()));
				min->setY(btMin(min->getY(),partMin.getY()));
				min->setZ(btMin(min->getZ(),partMin.getZ()));
				max->setX(btMax(max->getX(),partMax.getX()));
				max->setY(btMax(max->getY(),partMax.getY()));
				max->setZ(btMax(max->getZ(),partMax.getZ()));
			}
		}
	} walker(this, min, max);
	walker.walk(basic());
	const btTransform& transform = getTransform();
	*min = transform.invXform(*min);
	*max = transform.invXform(*max);
	//cerr << "Group min " << *min << " and max " << *max << endl;
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
			//cerr << "joint " << name << " to " << joint->name << endl;
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

Material::Material(zofColor c): color(c), density(1) {
	// Nothing more to do.
}

zofMaterial Material::asC() {
	return reinterpret_cast<zofMaterial>(this);
}

Material* Material::defaultMaterial() {
	static Material* material = new Material;
	return material;
}

Material* Material::of(zofMaterial material) {
	return reinterpret_cast<Material*>(material);
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

zofPart Part::asC() {
	return reinterpret_cast<zofPart>(this);
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
	//cerr << "Looking in " << this->name << " for joint " << name << endl;
	if (name.substr(0, 2) == "//") {
		// TODO Make path parsing more thorough. Base on CSS?
		string::size_type pos(name.find('/', 2));
		if (pos == string::npos) {
			// TODO Look for _any_ joint with this name?
			return 0;
		} else {
			string partName(name.substr(0, pos));
			string jointName(name.substr(pos + 1));
			//cerr << "Finding part " << partName << " for joint " << jointName << endl;
			Part* part = this->part(partName);
			return part ? part->joint(jointName) : 0;
		}
	}
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

Part* Part::of(zofPart part) {
	return reinterpret_cast<Part*>(part);
}

Part* Part::part(const string& name) {
	struct PartWalker: Walker {
		bool multi;
		const string& name;
		Part* part;
		PartWalker(const string& n): multi(false), name(n), part(0) {}
		void handle(BasicPart* part) {
			//cerr << "Is " << part->name << " == " << name << endl;
			if (part->name == name) {
				if (multi || this->part) {
					multi = true;
					this->part = 0;
				} else {
					this->part = part;
				}
			}
		}
	};
	// TODO Make path parsing more thorough. Base on CSS?
	//cerr << "Looking in " << this->name << " for " << name << endl;
	bool goDeep = name.substr(0,2) == "//";
	string partName(goDeep ? name.substr(2) : name);
	PartWalker walker(partName);
	if (goDeep) {
		//cerr << "Walking on " << walker.name << " - not " << name.substr(2) << "?" << endl;
		walker.walk(basic());
		//cerr << "Walked and found " << (walker.part ? walker.part->name : "nothing") << endl;
	} else {
		// Kids only.
		// TODO Make walker option for kids only and so on?
		for (map<string,Joint*>::iterator j = joints.begin(); j != joints.end(); j++) {
			Joint* joint = j->second;
			if (joint->other && joint->other->part) {
				walker.handle(joint->other->part->basic());
			}
		}
	}
	return walker.part;
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

zofSim Sim::asC() {
	return reinterpret_cast<zofSim>(this);
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

Sim* Sim::of(zofSim sim) {
	return reinterpret_cast<Sim*>(sim);
}

void Sim::update() {
	//	int ticks = 0;
	for (vector<Updater*>::iterator u = updaters.begin(); u < updaters.end(); u++) {
		(*u)->update(this);
	}
	dynamics->stepSimulation(btScalar(1)/btScalar(60));
	//	ticks++;
	//	if (ticks % 60 == 0) {
	//		cerr << '.';
	//	}
}

}
