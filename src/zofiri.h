#ifndef zofiri_h
#define zofiri_h

//#include <btBulletDynamicsCommon.h>
//#include <iostream>
//#include <map>
//#include <math.h>
//#include <vector>

using namespace std;

namespace zof {

// For circular references:
struct BodyInfo;
struct Command;
struct Hand;
struct Material;
struct MotionState;
struct Pub;
struct Sim;
struct Transaction;
struct Viz;
struct World;

struct Any {
	virtual ~Any();
};

}

#include "zof.h"

#endif
