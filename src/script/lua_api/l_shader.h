// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2025 Jules

#pragma once

#include "lua_api/l_base.h"

class ModApiShader : public ModApiBase {
private:
	static int l_register_shader(lua_State *L);
	static int l_set_shader_uniform(lua_State *L);
	static int l_get_shader_names(lua_State *L);

public:
	static void Initialize(lua_State *L, int top);
};
