#include "zofiri.h"
#include "world.h"

namespace zof {

Hand::Hand(World* w): world(w) {
	Sim* sim = world->sim;
	Material* material = new Material(0xFFFFFF00);
	//material->friction = ...
	carpal = sim->createBody(
		new btCapsuleShapeX(sim->cm(1.0),sim->cm(5.0)),
		btTransform(btQuaternion::getIdentity(), sim->m(btVector3(0.1,1,-0.1))),
		material
	);
	// Make the hand float using a kinematic metacarpal.
	// TODO Arm and body simulation with inverse kinematics or dynamics or whatnot.
	//metacarpal->setMassProps(0, btVector3(0,0,0));
	carpal->setCollisionFlags(carpal->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
	carpal->setActivationState(DISABLE_DEACTIVATION);
	carpal->setFriction(500);
	sim->dynamics->addRigidBody(carpal);
	for (int f = 0; f < 4; f++) {
		buildFinger(carpal, 4, sim->cm(btVector3(2.2 * (f-1.5), -1.1, 0)));
	}
	buildFinger(carpal, 3, sim->cm(btVector3(2.2 * (0.5), 0, -1.5)));
	buildFinger(carpal, 3, sim->cm(btVector3(2.2 * (-0.5), 0, -1.5)));
	// Open the hand at first.
	target = sim->m(2);
}

void Hand::buildFinger(btRigidBody* metacarpal, zof_uint count, const btVector3& position) {
	Sim* sim = world->sim;
	bool isThumb = count == 3;
	// The metacarpal is expected to be stretched along the z axis.
	btVector3& baseOrigin = metacarpal->getWorldTransform().getOrigin();
	btTransform constraintTransformA(
		//metacarpal->getWorldTransform().getRotation(),
		btQuaternion::getIdentity(),
		//btQuaternion(0,pi(0.5),pi(-0.5)),
		position
	);
	btTransform constraintTransformB(
		//metacarpal->getWorldTransform().getRotation(),
		btQuaternion::getIdentity(),
		sim->cm(btVector3(0, 2, 0))
	);
	btTransform transformB(
		btQuaternion::getIdentity(),
		constraintTransformA.getOrigin()
	);
	transformB.getOrigin() += baseOrigin - constraintTransformB.getOrigin();
	btVector3 hingeAxis(1,0,0);
	btRigidBody* a = metacarpal;
	for (zof_uint p = 0; p < count; p++) {
		btRigidBody* b = sim->createBody(new btCapsuleShape(sim->cm(1.0),sim->cm(1.5)), transformB, BodyInfo::of(a)->material);
		b->setFriction(500);
		//b->setDamping(0.5,0.5);
		sim->dynamics->addRigidBody(b);
		btHingeConstraint* hinge = new btHingeConstraint(
			*a, *b,
			constraintTransformA.getOrigin(), constraintTransformB.getOrigin(),
			hingeAxis, hingeAxis,
			false
		);
		btGeneric6DofConstraint* joint = new btGeneric6DofConstraint(
			*a, *b,
			constraintTransformA, constraintTransformB,
			false
		);
		// TODO Put spread hinges on metacarpals instead of fore/back.
		if (isThumb) {
			hinge->setLimit(pi(-0.3), pi(0.3));
		} else {
			hinge->setLimit(pi(-0.4), pi(0.1));
		}
		joint->setAngularLowerLimit(btVector3(pi(-0.4), pi(-0.01), pi(-0.1) * fabs(position.getX())/8));
		joint->setAngularUpperLimit(btVector3(pi(0.1), pi(0.01), pi(0.1) * fabs(position.getX())/8));
		if (isThumb) {
			joint->getRotationalLimitMotor(0)->m_loLimit = pi(-0.3);
			joint->getRotationalLimitMotor(0)->m_hiLimit = pi(0.3);
		}
		//joint->getRotationalLimitMotor(0)->m_damping = 0;
		//joint->getRotationalLimitMotor(0)->m_ERP = 0.01;
		// Curl in.
		hinge->enableMotor(true);
		hinge->setMaxMotorImpulse(400);
		//hinge->setMotorTarget(pi(-0.4), 0.1);
		joint->getRotationalLimitMotor(0)->m_enableMotor = true;
		joint->getRotationalLimitMotor(0)->m_maxMotorForce = 400;
		// Spread apart for ball grasp.
		joint->getRotationalLimitMotor(2)->m_enableMotor = true;
		joint->getRotationalLimitMotor(2)->m_maxMotorForce = 400;
		sim->dynamics->addConstraint(hinge);
		//sim->dynamics->addConstraint(joint);
		(isThumb ? thumbHinges : fingerHinges).push_back(hinge);
		(isThumb ? thumbJoints : fingerJoints).push_back(joint);
		a = b;
		transformB.getOrigin() += -2 * constraintTransformB.getOrigin();
		constraintTransformA.setOrigin(sim->cm(btVector3(0,-2,0)));
	}
}

bool Hand::grip() {
	const btVector3& handVel = carpal->getLinearVelocity();
	btScalar maxVel = 0;
	for (vector<btHingeConstraint*>::iterator j = fingerHinges.begin(); j < fingerHinges.end(); j++) {
		btHingeConstraint* joint = *j;
		joint->setMotorTarget(target < 0 ? joint->getLowerLimit() : joint->getUpperLimit(), 0.1);
		btScalar vel = (handVel - joint->getRigidBodyB().getLinearVelocity()).length();
		maxVel = zof_num_max(vel, maxVel);
	}
	for (vector<btHingeConstraint*>::iterator j = thumbHinges.begin(); j < thumbHinges.end(); j++) {
		btHingeConstraint* joint = *j;
		joint->setMotorTarget(target > 0 ? joint->getLowerLimit() : joint->getUpperLimit(), 0.1);
		btScalar vel = (handVel - joint->getRigidBodyB().getLinearVelocity()).length();
		maxVel = zof_num_max(vel, maxVel);
	}
	//	for (vector<btGeneric6DofConstraint*>::iterator j = fingerJoints.begin(); j < fingerJoints.end(); j++) {
	//		btGeneric6DofConstraint* joint = *j;
	//		joint->getRotationalLimitMotor(0)->m_targetVelocity = targetVel;
	//		btScalar vel = (handVel - joint->getRigidBodyB().getLinearVelocity()).length();
	//		maxVel = max_(vel, maxVel);
	//	}
	//	for (vector<btGeneric6DofConstraint*>::iterator j = thumbJoints.begin(); j < thumbJoints.end(); j++) {
	//		btGeneric6DofConstraint* joint = *j;
	//		joint->getRotationalLimitMotor(0)->m_targetVelocity = -targetVel;
	//		btScalar vel = (handVel - joint->getRigidBodyB().getLinearVelocity()).length();
	//		maxVel = max_(vel, maxVel);
	//	}
	// Test for convergence.
	// TODO Some memory to make sure it's consistent across time.
	// TODO Maybe some kind of controller class with active controller list?
	// TODO Distinguish true convergence vs. no-progress/error states.
	//cout << maxVel << "\n";
	return maxVel < world->sim->cm(4.0);
}

Stacker::Stacker(Hand* h): hand(h) {
	cargo = findBlock(0xFFFF0000);
	mode = OPEN;
	// TODO Use PD on the speed.
	speed = h->world->sim->cm(0.1);
}

void Stacker::act() {
	Sim& sim = *hand->world->sim;
	if (!cargo) {
		return;
	}
	//cout << "Acting!\n";
	// Get some hand information ready.
	btMotionState* handMotionState = hand->carpal->getMotionState();
	btTransform handTransform;
	handMotionState->getWorldTransform(handTransform);
	btVector3& handOrigin = handTransform.getOrigin();
	// See if we are over the cargo.
	btVector3 cargoDelta = cargo->getWorldTransform().getOrigin() - handOrigin;
	btBoxShape* shape = reinterpret_cast<btBoxShape*>(cargo->getCollisionShape());
	//cout << cargoDelta.y() << ", " << shape->getHalfExtentsWithoutMargin().y() << "\n";
	// TODO This doesn't deal with the case where the hand is _under_ the cargo.
	if (mode == CLOSED) {
		// The hand closing has converged. Lift it.
		if (handOrigin.getY() < sim.m(1.0)) {
			//cout << "Lifting!\n";
			handOrigin.setY(handOrigin.getY() + speed);
			handMotionState->setWorldTransform(handTransform);
		} else {
			if (trackToTarget(0xFF0000FF)) {
				hand->target = sim.m(2.0);
				mode = OPENING;
				btRigidBody* green = findBlock(0xFF00FF00);
				cargo = cargo == green ? 0 : green;
			}
		}
		if (cargoDelta.length() > 3 * shape->getHalfExtentsWithoutMargin().length()) {
			//cout << "Opening!\n";
			hand->target = sim.m(2.0);
			mode = OPENING;
		}
	} else if (cargo) {
		btVector3 deltaXz(cargoDelta.x(), 0, cargoDelta.z());
		btScalar dxz = deltaXz.length();
		if (dxz > sim.cm(0.1)) {
			// Move closer to the xz of the cargo.
			handOrigin += speed * deltaXz / dxz;
			handMotionState->setWorldTransform(handTransform);
		}
		if (dxz < sim.cm(1.0)) {
			// Mostly over the cargo. See if we are just a little over the cargo.
			btScalar dy = fabs(cargoDelta.y()) - shape->getHalfExtentsWithoutMargin().y();
			if (dy < sim.cm(6.0)) {
				// Just a little over it. Start closing the hand.
				mode = CLOSING;
				hand->target = sim.m(-4.0);
				if (hand->grip()) {
					//cout << "CLOSED!!!\n";
					mode = CLOSED;
				}
			}
			if (dy > sim.cm(4.0)) {
				// Keep lowering over the target.
				handOrigin.setY(handOrigin.getY() - speed);
				handMotionState->setWorldTransform(handTransform);
			}
		}
	}
	hand->grip();
}

btRigidBody* Stacker::findBlock(zof_color color) {
	for (vector<btRigidBody*>::iterator b = hand->world->blocks.begin(); b < hand->world->blocks.end(); b++) {
		btRigidBody* block = *b;
		zof_color blockColor = BodyInfo::of(block)->material->color;
		if (blockColor == color) {
			// Target the red block.
			return block;
		}
	}
	return 0;
}

bool Stacker::trackToTarget(zof_color color) {
	Sim& sim = *hand->world->sim;
	btTransform handTransform;
	btMotionState* handMotionState = hand->carpal->getMotionState();
	handMotionState->getWorldTransform(handTransform);
	btVector3& handOrigin = handTransform.getOrigin();
	btRigidBody* target = findBlock(color);
	btVector3 delta = target->getWorldTransform().getOrigin() - handOrigin;
	btVector3 deltaXz(delta.x(), 0, delta.z());
	btScalar dxz = deltaXz.length();
	if (dxz > sim.cm(0.1)) {
		// Move closer to the xz of the cargo.
		handOrigin += speed * deltaXz / dxz;
		handMotionState->setWorldTransform(handTransform);
		return false;
	} else {
		return true;
	}
}

World::World(Sim* s): sim(s) {
	btRigidBody* plane = sim->createPlane();
	sim->dynamics->addRigidBody(plane);
	// Tables.
	buildTable(sim->cm(btVector3(15,3,15)), sim->cm(btVector3(20,60,20)));
	buildTable(sim->cm(btVector3(25,3,25)), sim->cm(btVector3(-30,60,-30)));
	// Blocks.
	//cout << "Building blocks." << endl;
	buildBlock(0xFFFF0000, sim->cm(btVector3(15,67.25,15)));
	buildBlock(0xFF00FF00, sim->cm(btVector3(-15,67.25,-15)));
	buildBlock(0xFF0000FF, sim->cm(btVector3(-20,97.25,-20)));
	// Hand.
	hand = new Hand(this);
	stacker = new Stacker(hand);
	// Callback.
	sim->dynamics->setInternalTickCallback(updateWorld, this, true);
	// Reset to random state.
	reset();
}

void World::buildBlock(zof_color color, const btVector3& position) {
	Material* material = new Material(color);
	material->density = 0.1; // TODO Units!!!
    btVector3 halfExtents = sim->cm(btVector3(4.0, 4.0, 4.0));
	//cout << "block halfExtents: " << halfExtents.x() << " " << halfExtents.y() << " " << halfExtents.z() << endl;
    btRigidBody *block = sim->createBody(
    	new btBoxShape(halfExtents),
		btTransform(btQuaternion::getIdentity(), position),
		material
	);
	block->setFriction(500);
	sim->dynamics->addRigidBody(block);
	blocks.push_back(block);
}

void World::buildTable(const btVector3& halfExtents, const btVector3& position) {
	// Table.
	Material* wood = new Material(0xFF605020);
	btRigidBody* table = sim->createBody(
		new btBoxShape(halfExtents),
		btTransform(btQuaternion::getIdentity(), position),
		wood
	);
	table->setFriction(500);
	// The kinematic tables caused blocks to go crazy when more than one was in contact.
	//table->setCollisionFlags(table->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
	//table->setActivationState(DISABLE_DEACTIVATION);
	sim->dynamics->addRigidBody(table);
	// Legs.
	btScalar offset = sim->cm(0.25);
	btVector3 legSize(sim->cm(2.0), (position.y()-halfExtents.y())/2 - offset, sim->cm(2.0));
	//cout << "legSize: " << legSize.x() << ", " << legSize.y() << ", " << legSize.z() << "\n";
	btTransform constraintTransformTable(btQuaternion::getIdentity(), btVector3(0,0,0));
	btTransform constraintTransformLeg(btQuaternion::getIdentity(), btVector3(0,0,0));
	btScalar y = -legSize.y() - halfExtents.y();
	for (zof_int ix = -1; ix <= 1; ix += 2) {
		btScalar x = ix * (halfExtents.x() - legSize.x());
		for (zof_int iz = -1; iz <= 1; iz += 2) {
			btScalar z = iz * (halfExtents.z() - legSize.z());
			//cout << "leg: " << x << ", " << y << ", " << z << "\n";
			btVector3 legPosition(x,y,z);
			btRigidBody* leg = sim->createBody(
				new btBoxShape(legSize),
				btTransform(btQuaternion::getIdentity(), legPosition + position),
				wood
			);
			constraintTransformTable.setOrigin(btVector3(x, -halfExtents.y(), z));
			constraintTransformLeg.setOrigin(btVector3(0, legSize.y(), 0));
			btGeneric6DofConstraint* constraint = new btGeneric6DofConstraint(
				*table, *leg,
				constraintTransformTable, constraintTransformLeg,
				false
			);
			constraint->setAngularLowerLimit(btVector3(0,0,0));
			constraint->setAngularUpperLimit(btVector3(0,0,0));
			sim->dynamics->addConstraint(constraint, true);
			sim->dynamics->addRigidBody(leg);
		}
	}
	tables.push_back(table);
}

void World::reset() {
	// TODO Reset hand.
	// TODO hand->metacarpal->getMotionState()->getWorldTransform();
	// TODO Also reset state vars, etc.
	for (vector<btRigidBody*>::iterator b = blocks.begin(); b < blocks.end(); b++) {
		// Get the block info.
		btRigidBody* block = *b;
		btBoxShape* blockShape = reinterpret_cast<btBoxShape*>(block->getCollisionShape());
		btTransform blockTransform;
		block->getMotionState()->getWorldTransform(blockTransform);
		// Pick a table and get the info on it.
		size_t t = rand() % tables.size();
		btRigidBody* table = tables[t];
		btBoxShape* tableShape = reinterpret_cast<btBoxShape*>(table->getCollisionShape());
		btTransform tableTransform;
		table->getMotionState()->getWorldTransform(tableTransform);
		// Find a place over the table to drop the block.
		const btVector3& tableSize = tableShape->getHalfExtentsWithoutMargin();
		btScalar maxRadius = zof_num_min(tableSize.x(), tableSize.z()) - blockShape->getHalfExtentsWithoutMargin().x();
		btScalar radius = random(maxRadius);
		btScalar angle = pi(random(2.0));
		btScalar dx = cos(angle) * radius;
		btScalar dy = random(15.0) + 3 + tableSize.y() + blockShape->getHalfExtentsWithoutMargin().x();
		btScalar dz = sin(angle) * radius;
		// Place the block there.
		blockTransform.setOrigin(tableTransform.getOrigin() + sim->cm(btVector3(dx,dy,dz)));
		blockTransform.setRotation(btQuaternion(pi(random(2.0)),pi(random(2.0)),pi(random(2.0))));
		//cout << blockTransform.getOrigin().x() << ", " << blockTransform.getOrigin().y() << ", " << blockTransform.getOrigin().z() << "\n";
		block->getMotionState()->setWorldTransform(blockTransform);
		block->setWorldTransform(blockTransform);
		block->activate(true);
	}
}

void updateWorld(btDynamicsWorld *dynamics, btScalar timeStep) {
	World* world = reinterpret_cast<World*>(dynamics->getWorldUserInfo());
	world->stacker->act();
}

}
