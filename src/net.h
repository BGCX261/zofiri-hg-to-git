#ifndef zofiri_net_h
#define zofiri_net_h

#include "zofiri.h"
#include <string>
#include <vector>

#ifdef _WIN32
	// Include winsock stuff so we can use SOCKET even here.
	#include <winsock2.h>
	#include <windows.h>
#endif

using namespace std;

namespace zof {

// TODO Net struct for setup/teardown.
#ifdef _WIN32
	typedef SOCKET SocketId;
#else
	typedef int SocketId;
#endif

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
	 * Returns all socket ready for reading or null if none.
	 * It is not necessary to delete the sockets. They might be
	 * saved for future selections. Any open sockets will be
	 * closed when needed or when the server is closed.
	 *
	 * TODO Make this a callback, so we can check immediately on return?
	 */
	void select(vector<Socket*>* sockets);

private:

	// TODO Might have to generalize this a bit for windows.
	SocketId id;

	/**
	 * All currently open sockets from this server.
	 */
	vector<Socket*> sockets;

};

struct Socket {

	Socket(SocketId id);

	/**
	 * Closes the socket.
	 */
	~Socket();

	bool closed();

	/**
	 * Read all the data currently available.
	 *
	 * Return false only on end-of-stream.
	 */
	bool readAvailable(string* line, bool clear=true);

	/**
	 * Reads one line from the socket. The trailing LF or CRLF is not
	 * included in the string.
	 *
	 * Returns true if any data was read or false for an end-of-stream.
	 *
	 * TODO Move this feature set away from Socket.
	 */
	bool readLine(string* line, bool clear=true);

	/**
	 * Writes the null-terminated text followed by a newline.
	 *
	 * TODO Move this feature set away from Socket.
	 */
	void writeLine(const char* text);

private:

	friend struct Server;

	// TODO Might have to generalize this a bit for windows.
	SocketId id;

};

}

#endif
