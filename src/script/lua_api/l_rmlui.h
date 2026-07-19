// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "lua_api/l_base.h"

class ModApiRmlUi : public ModApiBase
{
private:
	// core.rmlui functions
	static int l_create(lua_State *L);
	static int l_destroy(lua_State *L);

	// ui document functions
	static int l_load(lua_State *L);
	static int l_load_string(lua_State *L);
	static int l_show(lua_State *L);
	static int l_hide(lua_State *L);
	static int l_close(lua_State *L);

	static int l_set_position(lua_State *L);
	static int l_set_size(lua_State *L);

	static int l_set_text(lua_State *L);
	static int l_set_html(lua_State *L);
	static int l_set_style(lua_State *L);
	static int l_set_attribute(lua_State *L);

	static int l_add_class(lua_State *L);
	static int l_remove_class(lua_State *L);

	static int l_find(lua_State *L);
	static int l_find_all(lua_State *L);

	static int l_focus(lua_State *L);
	static int l_blur(lua_State *L);

	static int l_reload(lua_State *L);
	static int l_bring_to_front(lua_State *L);
	static int l_capture(lua_State *L);
	static int l_call_js(lua_State *L);

public:
	static void Initialize(lua_State *L, int top);
};
