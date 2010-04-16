#include "zofiri.h"

namespace zofiri {

Hand::Hand(World* w): world(w) {
	Sim* sim = world->sim;
	Material* material = new Material(SColor(0xFFFFFF00));
	//material->friction = ...
	metacarpal = sim->createBody(
		new btCapsuleShapeX(1,5),
		btTransform(btQuaternion::getIdentity(), btVector3(10,100,-10)),
		material
	);
	// Make the hand float using a kinematic metacarpal.
	// TODO Arm and body simulation with inverse kinematics or dynamics or whatnot.
	//metacarpal->setMassProps(0, btVector3(0,0,0));
	metacarpal->setCollisionFlags(metacarpal->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
	metacarpal->setActivationState(DISABLE_DEACTIVATION);
	metacarpal->setFriction(500);
	sim->dynamics->addRigidBody(metacarpal);
	for (int f = 0; f < 4; f++) {
		buildFinger(metacarpal, 4, btVector3(2.2 * (f-1.5), -1.1, 0));
	}
	buildFinger(metacarpal, 3, btVector3(2.2 * (0.5), 0, -1.5));
	buildFinger(metacarpal, 3, btVector3(2.2 * (-0.5), 0, -1.5));
	// Open the hand at first.
	grip(2);
}

void Hand::buildFinger(btRigidBody* metacarpal, u32 count, const btVector3& position) {
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
		btVector3(0, 2, 0)
	);
	btTransform transformB(
		btQuaternion::getIdentity(),
		constraintTransformA.getOrigin()
	);
	transformB.getOrigin() += baseOrigin - constraintTransformB.getOrigin();
	btRigidBody* a = metacarpal;
	for (u32 p = 0; p < count; p++) {
		btRigidBody* b = sim->createBody(new btCapsuleShape(1,1.5), transformB, BodyInfo::of(a)->material);
		b->setFriction(500);
		//b->setDamping(0.5,0.5);
		sim->dynamics->addRigidBody(b);
		btGeneric6DofConstraint* joint = new btGeneric6DofConstraint(
			*a, *b,
			constraintTransformA, constraintTransformB,
			false
		);
		joint->setAngularLowerLimit(btVector3(pi(-0.4), pi(-0.01), pi(-0.1) * abs_(position.getX())/8));
		joint->setAngularUpperLimit(btVector3(pi(0.1), pi(0.01), pi(0.1) * abs_(position.getX())/8));
		if (isThumb) {
			joint->getRotationalLimitMotor(0)->m_loLimit = pi(-0.3);
			joint->getRotationalLimitMotor(0)->m_hiLimit = pi(0.3);
		}
		//joint->getRotationalLimitMotor(0)->m_damping = 0;
		//joint->getRotationalLimitMotor(0)->m_ERP = 0.01;
		// Curl in.
		joint->getRotationalLimitMotor(0)->m_enableMotor = true;
		joint->getRotationalLimitMotor(0)->m_maxMotorForce = 400;
		// Spread apart for ball grasp.
		joint->getRotationalLimitMotor(2)->m_enableMotor = true;
		joint->getRotationalLimitMotor(2)->m_maxMotorForce = 400;
		sim->dynamics->addConstraint(joint);
		(isThumb ? thumbJoints : fingerJoints).push_back(joint);
		a = b;
		transformB.getOrigin() += -2 * constraintTransformB.getOrigin();
		constraintTransformA.setOrigin(btVector3(0,-2,0));
	}
}

bool Hand::grip(btScalar targetVel) {
	const btVector3& handVel = metacarpal->getLinearVelocity();
	btScalar maxVel = 0;
	for (vector<btGeneric6DofConstraint*>::iterator j = fingerJoints.begin(); j < fingerJoints.end(); j++) {
		btGeneric6DofConstraint* joint = *j;
		joint->getRotationalLimitMotor(0)->m_targetVelocity = targetVel;
		btScalar vel = (handVel - joint->getRigidBodyB().getLinearVelocity()).length();
		maxVel = max_(vel, maxVel);
	}
	for (vector<btGeneric6DofConstraint*>::iterator j = thumbJoints.begin(); j < thumbJoints.end(); j++) {
		btGeneric6DofConstraint* joint = *j;
		joint->getRotationalLimitMotor(0)->m_targetVelocity = -targetVel;
		btScalar vel = (handVel - joint->getRigidBodyB().getLinearVelocity()).length();
		maxVel = max_(vel, maxVel);
	}
	// Test for convergence.
	// TODO Some memory to make sure it's consistent across time.
	// TODO Maybe some kind of controller class with active controller list?
	// TODO Distinguish true convergence vs. no-progress/error states.
	//cout << maxVel << "\n";
	return maxVel < 4;
}

Stacker::Stacker(Hand* h): hand(h) {
	cargo = findBlock(SColor(0xFFFF0000));
	mode = OPEN;
	// TODO Use PD on the speed.
	speed = 0.1;
}

void Stacker::act() {
	if (!cargo) {
		return;
	}
	//cout << "Acting!\n";
	// Get some hand information ready.
	btMotionState* handMotionState = hand->metacarpal->getMotionState();
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
		if (handOrigin.getY() < 100) {
			//cout << "Lifting!\n";
			handOrigin.setY(handOrigin.getY() + speed);
			handMotionState->setWorldTransform(handTransform);
		} else {
			if (trackToTarget(SColor(0xFF0000FF))) {
				hand->grip(2);
				mode = OPENING;
				btRigidBody* green = findBlock(SColor(0xFF00FF00));
				cargo = cargo == green ? 0 : green;
			}
		}
		if (cargoDelta.length() > 3 * shape->getHalfExtentsWithoutMargin().length()) {
			//cout << "Opening!\n";
			hand->grip(2);
			mode = OPENING;
		}
	} else if (cargo) {
		btVector3 deltaXz(cargoDelta.x(), 0, cargoDelta.z());
		btScalar dxz = deltaXz.length();
		if (dxz > 0.1) {
			// Move closer to the xz of the cargo.
			handOrigin += speed * deltaXz / dxz;
			handMotionState->setWorldTransform(handTransform);
		}
		if (dxz < 1) {
			// Mostly over the cargo. See if we are just a little over the cargo.
			btScalar dy = abs_(cargoDelta.y()) - shape->getHalfExtentsWithoutMargin().y();
			if (dy < 6) {
				// Just a little over it. Start closing the hand.
				mode = CLOSING;
				if (hand->grip(-4)) {
					//cout << "CLOSED!!!\n";
					mode = CLOSED;
				}
			}
			if (dy > 4) {
				// Keep lowering over the target.
				handOrigin.setY(handOrigin.getY() - speed);
				handMotionState->setWorldTransform(handTransform);
			}
		}
	}
}

btRigidBody* Stacker::findBlock(SColor color) {
	for (vector<btRigidBody*>::iterator b = hand->world->blocks.begin(); b < hand->world->blocks.end(); b++) {
		btRigidBody* block = *b;
		SColor blockColor = BodyInfo::of(block)->material->color;
		if (blockColor == color) {
			// Target the red block.
			return block;
		}
	}
	return 0;
}

bool Stacker::trackToTarget(SColor color) {
	btTransform handTransform;
	btMotionState* handMotionState = hand->metacarpal->getMotionState();
	handMotionState->getWorldTransform(handTransform);
	btVector3& handOrigin = handTransform.getOrigin();
	btRigidBody* target = findBlock(color);
	btVector3 delta = target->getWorldTransform().getOrigin() - handOrigin;
	btVector3 deltaXz(delta.x(), 0, delta.z());
	btScalar dxz = deltaXz.length();
	if (dxz > 0.1) {
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
	//buildTable(sim, btVector3(50,3,50), btVector3(0,60,0));
	buildTable(btVector3(15,3,15), btVector3(20,60,20));
	buildTable(btVector3(25,3,25), btVector3(-30,60,-30));
	// Blocks.
	buildBlock(SColor(0xFFFF0000), btVector3(15,67.25,15));
	buildBlock(SColor(0xFF00FF00), btVector3(-15,67.25,-15));
	buildBlock(SColor(0xFF0000FF), btVector3(-20,97.25,-20));
	// Hand.
	hand = new Hand(this);
	stacker = new Stacker(hand);
	// Callback.
	sim->dynamics->setInternalTickCallback(updateWorld, this, true);
	// Reset to random state.
	reset();
}

void World::buildBlock(SColor color, const btVector3& position) {
	Material* material = new Material(color);
	material->density = 0.1;
	btRigidBody* block = sim->createBody(
		new btBoxShape(btVector3(4,4,4)),
		btTransform(btQuaternion::getIdentity(), position),
		material
	);
	block->setFriction(500);
	sim->dynamics->addRigidBody(block);
	blocks.push_back(block);
}

void World::buildTable(const btVector3& halfExtents, const btVector3& position) {
	// Table.
	Material* wood = new Material(SColor(0xFF605020));
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
	btScalar offset = 0.25;
	btVector3 legSize(2, (position.y()-halfExtents.y())/2 - offset, 2);
	//cout << "legSize: " << legSize.x() << ", " << legSize.y() << ", " << legSize.z() << "\n";
	btTransform constraintTransformTable(btQuaternion::getIdentity(), btVector3(0,0,0));
	btTransform constraintTransformLeg(btQuaternion::getIdentity(), btVector3(0,0,0));
	btScalar y = -legSize.y() - halfExtents.y();
	for (s32 ix = -1; ix <= 1; ix += 2) {
		btScalar x = ix * (halfExtents.x() - legSize.x());
		for (s32 iz = -1; iz <= 1; iz += 2) {
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
		btScalar maxRadius = min_(tableSize.x(), tableSize.z()) - blockShape->getHalfExtentsWithoutMargin().x();
		btScalar radius = drand48() * maxRadius;
		btScalar angle = pi(2 * drand48());
		btScalar dx = cos(angle) * radius;
		btScalar dy = 15 * drand48() + 3 + tableSize.y() + blockShape->getHalfExtentsWithoutMargin().x();
		btScalar dz = sin(angle) * radius;
		// Place the block there.
		blockTransform.setOrigin(tableTransform.getOrigin() + btVector3(dx,dy,dz));
		blockTransform.setRotation(btQuaternion(pi(2*drand48()),pi(2*drand48()),pi(2*drand48())));
		cout << blockTransform.getOrigin().x() << ", " << blockTransform.getOrigin().y() << ", " << blockTransform.getOrigin().z() << "\n";
		block->getMotionState()->setWorldTransform(blockTransform);
		block->setWorldTransform(blockTransform);
	}
}

void updateWorld(btDynamicsWorld *dynamics, btScalar timeStep) {
	World* world = reinterpret_cast<World*>(dynamics->getWorldUserInfo());
	world->stacker->act();
}

}
