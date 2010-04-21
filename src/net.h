#ifndef zofiri_net_h
#define zofiri_net_h

#include <string>

using namespace std;

namespace zof {

struct Socket;

struct Server {

	Server(int port);

	/**
	 * Closes the server automatically.
	 */
	~Server();

	Socket* accept();

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
