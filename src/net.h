#ifndef zofiri_net_h
#define zofiri_net_h

#include <string>

using namespace std;

namespace zof {

// TODO Net struct for setup/teardown.

struct Socket;

struct Server {

	Server(int port);

	/**
	 * Closes the server automatically.
	 */
	~Server();

	/**
	 * Hang waiting for a socket.
	 */
	Socket* accept();

	/**
	 * Returns a socket ready for reading or null if none.
	 * It is not necessary to delete the socket. It might be
	 * saved for future selections. Any open sockets will be
	 * closed when needed or when the server is closed.
	 *
	 * TODO Make this a callback, so we can check immediately on return?
	 */
	Socket* select();

private:

	// TODO Might have to generalize this a bit for windows.
	int id;

};

struct Socket {

	Socket(int id);

	/**
	 * Closes the socket.
	 */
	~Socket();

	/**
	 * Read all the data currently available.
	 *
	 * Return false only on end-of-stream.
	 */
	bool readAvailable(std::string& line, bool clear=true);

	/**
	 * Reads one line from the socket. The trailing LF or CRLF is not
	 * included in the string.
	 *
	 * Returns true if any data was read or false for an end-of-stream.
	 *
	 * TODO Move this feature set away from Socket.
	 */
	bool readLine(std::string& line, bool clear=true);

	/**
	 * Writes the null-terminated text followed by a newline.
	 *
	 * TODO Move this feature set away from Socket.
	 */
	void writeLine(char* text);

private:

	// TODO Might have to generalize this a bit for windows.
	int id;

};

}

#endif
