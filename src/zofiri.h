#ifndef zofiri_h
#define zofiri_h

#include <btBulletDynamicsCommon.h>
#include <iostream>
#include <irrlicht.h>
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
struct Hand;
struct Material;
struct MotionState;
struct Sim;
struct Viz;
struct World;

}

#include "net.h"
#include "sim.h"
#include "util.h"
#include "viz.h"
#include "world.h"

#endif
