// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2024 Jules

#include "l_shader.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "l_internal.h"
#include "server.h"
#include "filesys.h"

#if CHECK_CLIENT_BUILD()
#include "client/client.h"
#include "client/shader.h"
#endif

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

	std::string filename = fs::GetFilenameFromPath(path.c_str());

	if (stage == "vertex") {
		ov.vertex_path = filename;
	} else if (stage == "fragment") {
		ov.fragment_path = filename;
	} else {
		ov.vertex_path = filename;
		ov.fragment_path = filename;
	}

#if CHECK_CLIENT_BUILD()
	Client *client = getClient(L);
	if (client) {
		IWritableShaderSource *shsrc = client->getShaderSource();
		if (shsrc)
			shsrc->registerModShader(ov);
		return 0;
	}
#endif

	Server *server = getServer(L);
	if (server)
		server->registerModShader(ov);

	return 0;
}

// core.set_shader_uniform(shader_name, uniform_name, value)
int ModApiShader::l_set_shader_uniform(lua_State *L)
{
	std::string shader_name = luaL_checkstring(L, 1);
	std::string uniform_name = luaL_checkstring(L, 2);

	UniformValue val;
	if (lua_isnumber(L, 3)) {
		double n = lua_tonumber(L, 3);
		if (n == (int)n)
			val = (int)n;
		else
			val = (float)n;
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

#if CHECK_CLIENT_BUILD()
	Client *client = getClient(L);
	if (client) {
		IWritableShaderSource *shsrc = client->getShaderSource();
		if (shsrc)
			shsrc->setModShaderUniform(shader_name, uniform_name, val);
		return 0;
	}
#endif

	Server *server = getServer(L);
	if (server)
		server->setModShaderUniform(shader_name, uniform_name, val);

	return 0;
}

// core.get_shader_names()
int ModApiShader::l_get_shader_names(lua_State *L)
{
#if CHECK_CLIENT_BUILD()
	Client *client = getClient(L);
	if (client) {
		IWritableShaderSource *shsrc = client->getShaderSource();
		if (shsrc) {
			std::vector<std::string> names = shsrc->getOverridableShaderNames();

			lua_newtable(L);
			for (size_t i = 0; i < names.size(); i++) {
				lua_pushstring(L, names[i].c_str());
				lua_rawseti(L, -2, i + 1);
			}
			return 1;
		}
	}
#endif

	lua_newtable(L);
	for (size_t i = 0; i < overridable_shaders.size(); i++) {
		lua_pushstring(L, overridable_shaders[i].c_str());
		lua_rawseti(L, -2, i + 1);
	}
	return 1;
}

void ModApiShader::Initialize(lua_State *L, int top)
{
	API_FCT(register_shader);
	API_FCT(set_shader_uniform);
	API_FCT(get_shader_names);

	// Also register in minetest table for robustness
	lua_getglobal(L, "minetest");
	if (lua_istable(L, -1)) {
		int mt_top = lua_gettop(L);
		registerFunction(L, "register_shader", l_register_shader, mt_top);
		registerFunction(L, "set_shader_uniform", l_set_shader_uniform, mt_top);
		registerFunction(L, "get_shader_names", l_get_shader_names, mt_top);
	}
	lua_pop(L, 1);
}
