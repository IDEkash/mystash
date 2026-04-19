// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "l_base.h"

class ModApiCamera : public ModApiBase
{
private:
	static int l_get_rotation(lua_State *L);
	static int l_set_rotation(lua_State *L);
	static int l_add_rotation(lua_State *L);
	static int l_lerp_rotation(lua_State *L);
	static int l_reset(lua_State *L);

	static int l_get_fov(lua_State *L);
	static int l_set_fov(lua_State *L);
	static int l_lerp_fov(lua_State *L);

	static int l_get_mode(lua_State *L);
	static int l_set_mode(lua_State *L);

	static int l_get_position(lua_State *L);
	static int l_set_position(lua_State *L);
	static int l_add_position(lua_State *L);
	static int l_lerp_position(lua_State *L);
	static int l_reset_position(lua_State *L);

public:
	static void Initialize(lua_State *L, int top);
};
