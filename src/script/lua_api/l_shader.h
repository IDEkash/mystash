// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2024 Jules

#pragma once

#include "lua_api/l_base.h"

class ModApiShader : public ModApiBase {
public:
	static void Initialize(lua_State *L, int top);

private:
	// core.register_shader(def)
	static int l_register_shader(lua_State *L);

	// core.set_shader_uniform(shader_name, uniform_name, value)
	static int l_set_shader_uniform(lua_State *L);

	// core.get_shader_names()
	static int l_get_shader_names(lua_State *L);
};
