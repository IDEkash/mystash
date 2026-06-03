// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "lua_api/l_base.h"

class ModApiJav : public ModApiBase {
private:
	static int l_import(lua_State *L);
	static int l_new(lua_State *L);
	static int l_call(lua_State *L);
	static int l_get(lua_State *L);
	static int l_set(lua_State *L);
	static int l_methods(lua_State *L);
	static int l_fields(lua_State *L);
	static int l_classes(lua_State *L);
	static int l_help(lua_State *L);
	static int l_async(lua_State *L);
	static int l_on(lua_State *L);

public:
	static void Initialize(lua_State *L, int top);
	static void Poll(lua_State *L);
};
