// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2024 Jules

#include "l_shader.h"
#include "client/client.h"
#include "client/shader.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "l_internal.h"
#include "lua_api/l_client.h"

// core.register_shader(def)
int ModApiShader::l_register_shader(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TTABLE);

	ModShaderOverride ov;
	ov.name = getstringfield_default(L, 1, "name", "");
	ov.target = getstringfield_default(L, 1, "target", "");
	ov.priority = getintfield_default(L, 1, "priority", 0);

	std::string stage = getstringfield_default(L, 1, "stage", "both");
	std::string path = getstringfield_default(L, 1, "path", "");

	if (ov.name.empty() || ov.target.empty() || path.empty()) {
		return 0;
	}

	if (stage == "vertex") {
		ov.vertex_path = path;
	} else if (stage == "fragment") {
		ov.fragment_path = path;
	} else {
		// "both" or default
		// We expect the path to be a base, and we append suffixes?
		// No, the example shows one path for one stage.
		// If stage is "both", it probably means the same file for both or it's an error in my understanding of the requirement.
		// "stage = "fragment"" in example.
		// If "both", maybe it means we should have two paths?
		// Actually, let's look at the requirement:
		// stage: "vertex", "fragment", or "both"
		// If "both", I'll use it for both for now, but usually they are different.
		ov.vertex_path = path;
		ov.fragment_path = path;
	}

	getClient(L)->getShaderSource()->registerModShader(ov);

	return 0;
}

// core.set_shader_uniform(shader_name, uniform_name, value)
int ModApiShader::l_set_shader_uniform(lua_State *L)
{
	std::string shader_name = luaL_checkstring(L, 1);
	std::string uniform_name = luaL_checkstring(L, 2);

	UniformValue val;
	if (lua_isnumber(L, 3)) {
		val = (float)lua_tonumber(L, 3);
	} else if (lua_isboolean(L, 3)) {
		val = (int)lua_toboolean(L, 3);
	} else if (lua_istable(L, 3)) {
		lua_getfield(L, 3, "z");
		bool has_z = !lua_isnil(L, -1);
		lua_pop(L, 1);

		if (has_z) {
			val = read_v3f(L, 3);
		} else {
			lua_getfield(L, 3, "y");
			bool has_y = !lua_isnil(L, -1);
			lua_pop(L, 1);
			if (has_y) {
				val = read_v2f(L, 3);
			} else {
				return 0;
			}
		}
	} else {
		return 0;
	}

	getClient(L)->getShaderSource()->setModShaderUniform(shader_name, uniform_name, val);

	return 0;
}

// core.get_shader_names()
int ModApiShader::l_get_shader_names(lua_State *L)
{
	std::vector<std::string> names = getClient(L)->getShaderSource()->getOverridableShaderNames();

	lua_newtable(L);
	for (size_t i = 0; i < names.size(); i++) {
		lua_pushstring(L, names[i].c_str());
		lua_rawseti(L, -2, i + 1);
	}
	return 1;
}

void ModApiShader::Initialize(lua_State *L, int top)
{
	API_FCT(register_shader);
	API_FCT(set_shader_uniform);
	API_FCT(get_shader_names);
}
