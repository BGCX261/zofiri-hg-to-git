#ifndef hum_control_h
#define hum_control_h

#include "bot.h"

namespace hum {

struct Controller {

	Controller(Bot* bot);

	void update();

	Bot* bot;

	zofM3 goalPos;

	zofM3Rat goalRot;

};

}

#endif
