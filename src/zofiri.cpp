#include "zofiri.h"

using namespace zofiri;

int main(int argc, char** argv) {
	srand(time(0));
	srand48(time(0));
	Sim sim;
	World world(&sim);
	Viz viz(&sim);
	viz.run();
}

// One round on the compiler is faster than multiple.
// But if these grow much, then it might be better to make a pass on each file.
#include "sim.cpp"
#include "viz.cpp"
#include "world.cpp"
