#ifndef zofiri_h
#define zofiri_h

#include <btBulletDynamicsCommon.h>
#include <iostream>
#include <irrlicht.h>
#include <map>
#include <math.h>
#include <vector>

using namespace irr;
using namespace std;

using namespace core;
using namespace io;
using namespace scene;
using namespace video;

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

}

#include "mat.h"
#include "net.h"
#include "pub.h"
#include "sim.h"
#include "viz.h"
#include "world.h"

#endif
