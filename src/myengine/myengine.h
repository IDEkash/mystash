#pragma once

#include "config.h"

extern "C" {
#include <lua.h>
}

namespace MyEngine {
	void initialize(lua_State *L);
}
