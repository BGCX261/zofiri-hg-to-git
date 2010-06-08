#ifndef zofiri_h
#define zofiri_h

#include <btBulletDynamicsCommon.h>
#include <iostream>
#include <map>
#include <math.h>
#include <vector>

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

}

#include "zof.h"

typedef struct {
	zof_int sig;
} zof_meta;

#include "mat.h"
#include "net.h"
#include "pub.h"
#include "sim.h"
#include "viz.h"

#include "world.h"

#endif
