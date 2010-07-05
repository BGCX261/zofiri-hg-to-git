#include "zofiri.h"
#include "mat.h"
#include "sim.h"
#include <btBulletDynamicsCommon.h>
#include <vector>

#ifndef zofiri_world_h
#define zofiri_world_h

namespace zof {

void updateWorld(btDynamicsWorld *dynamics, btScalar timeStep);

struct Hand {

	Hand(World* world);

	void buildFinger(btRigidBody* metacarpal, zofUint count, const btVector3& position);

	vector<btHingeConstraint*> fingerHinges;
	vector<btGeneric6DofConstraint*> fingerJoints;

	/**
	 * Set positive targetVel to open, negative to close.
	 */
	bool grip();

	btRigidBody* carpal;

	btScalar target;

	vector<btHingeConstraint*> thumbHinges;
	vector<btGeneric6DofConstraint*> thumbJoints;

	World* world;

};

struct Stacker {

	enum Mode {OPENING, OPEN, CLOSING, CLOSED};

	Stacker(Hand* hand);

	void act();

	btRigidBody* cargo;

	btRigidBody* findBlock(zofColor color);

	Hand* hand;

	bool trackToTarget(zofColor color);

	Mode mode;

	btScalar speed;

};

/**
 * Specific world for grasping/stacking demo.
 */
struct World {

	World(Sim* sim);

	void buildBlock(zofColor color, const btVector3& position);

	void buildTable(const btVector3& halfExtents, const btVector3& position);

	vector<btRigidBody*> blocks;

	Hand* hand;

	void reset();

	Sim* sim;

	vector<btRigidBody*> tables;

	Stacker* stacker;

};

}

#endif
