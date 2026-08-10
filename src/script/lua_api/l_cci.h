#pragma once

#include "lua_api/l_base.h"

class ModApiCCI : public ModApiBase {
private:
	// core.cci_register_style(style_name, def)
	static int l_cci_register_style(lua_State *L);

	// core.cci_create_instance(instance_name, style_name, properties)
	static int l_cci_create_instance(lua_State *L);

	// core.cci_destroy_instance(instance_name, player_name)
	static int l_cci_destroy_instance(lua_State *L);

public:
	static void Initialize(lua_State *L, int top);
};
