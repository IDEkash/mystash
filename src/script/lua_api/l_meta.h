// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "lua_api/l_base.h"

class ModApiMeta : public ModApiBase {
public:
	static void Initialize(lua_State *L, int top);

private:
	static int l_engine_get(lua_State *L);
	static int l_engine_list(lua_State *L);
	static int l_hook(lua_State *L);
	static int l_override(lua_State *L);
};
