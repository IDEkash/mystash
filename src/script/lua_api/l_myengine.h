// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "lua_api/l_base.h"

class ModApiMyEngine : public ModApiBase {
public:
	static lua_State* getLuaState();
private:
	static int l_get(lua_State *L);
	static int l_set(lua_State *L);
	static int l_hook(lua_State *L);
	static int l_modify(lua_State *L);
	static int l_rewrite(lua_State *L);
	static int l_add(lua_State *L);
	static int l_remove(lua_State *L);
	static int l_update(lua_State *L);
	static int l_watch(lua_State *L);

public:
	static void Initialize(lua_State *L, int top);
	static void resetAll();
	static bool hasRewrite(const std::string &path);
	static int getRewrite(const std::string &path);
	static int getModify(const std::string &path);
};
