// Luanti Universal Engine Extension Architecture
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "lua_api/l_base.h"
#include "unapi_types.h"

class ModApiUnapi : public ModApiBase {
private:
	static int l_inspect(lua_State *L);
	static int l_create(lua_State *L);
	static int l_resolve(lua_State *L);
	static int l_hook(lua_State *L);
	static int l_unhook(lua_State *L);
	static int l_list_types(lua_State *L);
	static int l_list_hooks(lua_State *L);
	static int l_storage_get(lua_State *L);
	static int l_network_http_fetch(lua_State *L);

	// Object methods on wrapped extension object instances
	static int l_object_get_property(lua_State *L);
	static int l_object_set_property(lua_State *L);
	static int l_object_invoke_method(lua_State *L);
	static int l_object_get_child(lua_State *L);
	static int l_object_get_handle(lua_State *L);
	static int l_object_get_info(lua_State *L);
	static int l_object_index(lua_State *L);
	static int l_object_newindex(lua_State *L);
	static int l_object_eq(lua_State *L);
	static int l_object_tostring(lua_State *L);
	static int l_object_gc(lua_State *L);

public:
	static void Initialize(lua_State *L, int top);
	static void pushValueToLua(lua_State *L, const unapi::Value &val);
	static unapi::Value luaToValue(lua_State *L, int index);
};
