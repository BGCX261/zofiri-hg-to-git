#include "zofiri.h"

#ifndef zofiri_world_h
#define zofiri_world_h

namespace zofiri {

void updateWorld(btDynamicsWorld *dynamics, btScalar timeStep);

struct Hand {

	Hand(World* world);

	void buildFinger(btRigidBody* metacarpal, u32 count, const btVector3& position);

	vector<btHingeConstraint*> fingerHinges;
	vector<btGeneric6DofConstraint*> fingerJoints;

	/**
	 * Set positive targetVel to open, negative to close.
	 */
	bool grip(btScalar targetVel);

	btRigidBody* carpal;

	vector<btHingeConstraint*> thumbHinges;
	vector<btGeneric6DofConstraint*> thumbJoints;

	World* world;

};

struct Stacker {

	enum Mode {OPENING, OPEN, CLOSING, CLOSED};

	Stacker(Hand* hand);

	void act();

	btRigidBody* cargo;

	btRigidBody* findBlock(SColor color);

	Hand* hand;

	bool trackToTarget(SColor color);

	Mode mode;

	btScalar speed;

};

/**
 * Specific world for grasping/stacking demo.
 */
struct World {

	World(Sim* sim);

	void buildBlock(SColor color, const btVector3& position);

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
