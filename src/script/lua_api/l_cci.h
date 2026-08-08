// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Author: Jules

#pragma once

#include "lua_api/l_base.h"

class ModApiCCI : public ModApiBase {
public:
	static void Initialize(lua_State *L, int top);
};
