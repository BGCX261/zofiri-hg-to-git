#ifndef zofiri_h
#define zofiri_h

namespace zof {

// For circular references:
struct BasicPart;
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
