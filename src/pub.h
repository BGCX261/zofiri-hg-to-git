#ifndef zof_pub_h
#define zof_pub_h

#include "zofiri.h"
#include "net.h"
#include "viz.h"
#include <map>
#include <string>
#include <vector>

namespace zof {

/**
 * The main zof server that publishes the simulation, hence "pub".
 */
struct zofExport Pub {

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
	map<string, Command*> commands;

	/**
	 * TODO Just a hack for now for testing. Clean all this up.
	 */
	zofString modUri;

	/**
	 * Reads incoming messages, if any, and updates the sim or viz.
	 * If no messages, just returns right away.
	 */
	void update();

	Viz* viz;

private:

	Server* server;

};

struct Command {
	/**
	 * All arguments are presumed substituted with vars by now.
	 * TODO Build out a full process model with IO streams?
	 * TODO That might likely require separate threads.
	 * TODO For now, just assume short IO, therefore strings.
	 */
	virtual string perform(Transaction* tx) = 0;
};

/**
 * Provides the context of a series of commands.
 * TODO I'm not convinced I'm committed supporting atomic transactions, though.
 */
struct Transaction {

	Transaction(Pub* pub);

	/**
	 * Filled with substituted args before each command is performed.
	 * Doesn't contain var assignment.
	 * _Does_ contain the command name.
	 */
	vector<string> args;

	/**
	 * Already broken into words, but var substitution is still performed.
	 * This also checks for and performs var assignment.
	 */
	string processCommand(const vector<string>& args);

	/**
	 * Breaks apart the line then calls processCommand.
	 */
	string processLine(const string& line);

	Pub* pub;

	/**
	 * Each transaction can carry its own local vars to avoid round trips
	 * when composing data across multiple commands.
	 */
	map<string,string> vars;

};

}

#endif
