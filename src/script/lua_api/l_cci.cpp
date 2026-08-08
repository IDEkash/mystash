// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Author: Jules

#include "lua_api/l_cci.h"
#include "lua_api/l_internal.h"
#include <cmath>
#include <algorithm>

int ModApiCCI::l_draw_line(lua_State *L)
{
	float x1 = luaL_checknumber(L, 1);
	float y1 = luaL_checknumber(L, 2);
	float x2 = luaL_checknumber(L, 3);
	float y2 = luaL_checknumber(L, 4);
	std::string color = luaL_checkstring(L, 5);

	float dx = x2 - x1;
	float dy = y2 - y1;
	float dist = std::sqrt(dx*dx + dy*dy);
	float step = 0.12f;
	int steps = std::max(1, (int)std::floor(dist / step));

	lua_newtable(L);
	for (int i = 0; i <= steps; ++i) {
		float t = (float)i / steps;
		float px = x1 + dx * t;
		float py = y1 + dy * t;
		char buf[128];
		std::snprintf(buf, sizeof(buf), "box[%f,%f;0.06,0.06;%s]", px - 0.03f, py - 0.03f, color.c_str());
		lua_pushstring(L, buf);
		lua_rawseti(L, -2, i + 1);
	}
	return 1;
}

void ModApiCCI::Initialize(lua_State *L, int top)
{
	lua_newtable(L);
	int tbl = lua_gettop(L);

	registerFunction(L, "draw_line_native", l_draw_line, tbl);

	lua_pushvalue(L, tbl);
	lua_setglobal(L, "cci");
	lua_setfield(L, top, "cci");
}
