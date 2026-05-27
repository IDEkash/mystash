// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "lua_api/l_base.h"

class ModApiDim : public ModApiBase {
private:
	static int l_create_world(lua_State *L);
	static int l_delete_world(lua_State *L);
	static int l_enter_world(lua_State *L);
	static int l_set_visible(lua_State *L);
	static int l_link_mod(lua_State *L);

public:
	static void Initialize(lua_State *L, int top);
};
