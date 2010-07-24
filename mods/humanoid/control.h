#ifndef hum_control_h
#define hum_control_h

#include "bot.h"

namespace hum {

struct Controller {

	Controller(Humanoid* humanoid);

	void update();

	Humanoid* humanoid;

};

}

#endif
