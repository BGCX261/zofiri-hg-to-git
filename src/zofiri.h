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

namespace zofiri {

struct BodyInfo;
struct Hand;
struct Material;
struct MotionState;
struct Sim;
struct Viz;
struct World;

f64 pi(f64 scale = 1) {
	return scale * 3.14159265358979323846;
}

f64 radToDeg(f64 rad) {
	return 180 * rad / pi();
}

template<typename Num>
Num sign(Num n) {
	return n ? (n > 0 ? 1 : -1) : 0;
}

}

#include "sim.h"
#include "viz.h"
#include "world.h"

#endif
