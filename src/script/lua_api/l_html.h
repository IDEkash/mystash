// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "lua_api/l_base.h"
#include "util/html.h"

class ModApiHTML : public ModApiBase
{
private:
	// html.parse(text)
	static int l_parse(lua_State *L);

	// html.stream()
	static int l_stream(lua_State *L);

public:
	static void Initialize(lua_State *L, int top);
};

class LuaHTMLStream : public ModApiBase
{
private:
	static const luaL_Reg methods[];

	// garbage collector
	static int gc_object(lua_State *L);

	// feed(self, text)
	static int l_feed(lua_State *L);

	// end(self)
	static int l_end(lua_State *L);

	// get_root(self)
	static int l_get_root(lua_State *L);

	std::unique_ptr<html::Parser> m_parser;

public:
	LuaHTMLStream();
	~LuaHTMLStream() = default;

	static void Register(lua_State *L);
	static int create_object(lua_State *L);

	static const char className[];
};
