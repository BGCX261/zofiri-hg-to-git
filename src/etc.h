/**
 * Just a miscellaneous utility area. Move out any groups from
 * here that seem to belong together as a common theme and are big
 * enough to warrant that.
 */
#ifndef zof_etc_h
#define zof_etc_h

#include <sstream>
#include <string>

using namespace std;

namespace zof {

/**
 * Word around town is that the best way to parse out strings in
 * C++ is stringstream, and that's awkward for quick use.
 * Instead, read straight from strings. At least my operator definition
 * is under the zof namespace here, so I'm not polluting generally.
 */
template<typename Result>
void operator>>(string& in, Result& result) {
	stringstream stream(in);
	stream >> result;
}

}

#endif
