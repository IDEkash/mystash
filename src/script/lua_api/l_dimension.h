// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "lua_api/l_base.h"

class ModApiDimension : public ModApiBase {
private:
	static int l_create(lua_State *L);
	static int l_delete(lua_State *L);
	static int l_list(lua_State *L);
	static int l_exists(lua_State *L);
	static int l_get_info(lua_State *L);
	static int l_transfer_player(lua_State *L);
	static int l_set_visible(lua_State *L);
	static int l_link_mod(lua_State *L);

public:
	static void Initialize(lua_State *L, int top);
};
