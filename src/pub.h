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

	/**
	 * All command objects are deleted on destruct.
	 * Remove first if you don't want that.
	 */
	std::map<std::string, Command*> commands;

	void processCommand(const vector<std::string>& args);

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

struct Command {
	/**
	 * TODO Build out a full process model with IO streams?
	 * TODO That might likely require separate threads.
	 * TODO For now, just assume short IO, therefore strings.
	 */
	virtual std::string perform(const vector<std::string>& args) = 0;
};

}

#endif
