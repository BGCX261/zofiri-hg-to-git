#ifndef zof_pub_h
#define zof_pub_h

#include "zofiri.h"

namespace zof {

/**
 * The main zof server that publishes the simulation, hence "pub".
 */
struct Pub {

	/**
	 * Expose the viz here for now, so can manipulate both the
	 * display and the simulation. The viz exposes the sim, so both
	 * are available.
	 *
	 * TODO Consider an App for wrapping everything sometime.
	 */
	Pub(Viz* viz, int port = 2041);

	~Pub();

	void processCommand(const vector<std::string*>& words);

	void processLine(const std::string& line);

	/**
	 * Reads incoming messages, if any, and updates the sim or viz.
	 * If no messages, just returns right away.
	 */
	void update();

	Viz* viz;

	/**
	 * TODO Delete this once "world" is gone. Everything just needs to be in sim.
	 */
	World* world;

private:

	Server* server;

};

}

#endif
