// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2025 Jules

#include "l_shader.h"
#include "l_internal.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#if CHECK_CLIENT_BUILD()
#include "client/client.h"
#endif
#include "client/shader.h"
#include "filesys.h"
#include "porting.h"
#include "content/mods.h"
#include "server.h"

// core.register_shader(def)
int ModApiShader::l_register_shader(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TTABLE);

	ModShaderOverride ov;
	ov.name = getstringfield_default(L, 1, "name", "");
	ov.target = getstringfield_default(L, 1, "target", "");
	ov.stage = getstringfield_default(L, 1, "stage", "both");
	ov.priority = getintfield_default(L, 1, "priority", 0);

	std::string raw_path = getstringfield_default(L, 1, "path", "");
	if (raw_path.empty()) {
		return 0;
	}

	std::string absolute_path = raw_path;
	size_t colon_pos = raw_path.find(':');
	// Mod path resolution: "modname:path/to/file"
	// On Windows, absolute paths like "C:\path" also contain a colon.
	// Mod names only allow [a-z0-0_], so a single char followed by a colon
	// is likely a drive letter.
	if (colon_pos != std::string::npos && colon_pos > 1) {
		std::string modname = raw_path.substr(0, colon_pos);
		std::string relative_path = raw_path.substr(colon_pos + 1);
		const ModSpec *spec = nullptr;
#if CHECK_CLIENT_BUILD()
		if (getScriptApiBase(L)->getType() == ScriptingType::Client)
			spec = getClient(L)->getModSpec(modname);
		else
#endif
			spec = getServer(L)->getModSpec(modname);

		if (spec) {
			absolute_path = spec->path + DIR_DELIM + relative_path;
		}
	}
	ov.path = absolute_path;

#if CHECK_CLIENT_BUILD()
	if (getScriptApiBase(L)->getType() == ScriptingType::Client) {
		IWritableShaderSource *shdsrc = getClient(L)->getShaderSource();
		if (shdsrc)
			shdsrc->registerShaderOverride(ov);
	} else
#endif
	{
		getServer(L)->registerShaderOverride(ov);
	}

	return 0;
}

// core.set_shader_uniform(shader_name, uniform_name, value)
int ModApiShader::l_set_shader_uniform(lua_State *L)
{
	std::string shader_name = luaL_checkstring(L, 1);
	std::string uniform_name = luaL_checkstring(L, 2);

	ModUniformValue val;
	if (lua_isnumber(L, 3)) {
		val = (float)lua_tonumber(L, 3);
	} else if (lua_isboolean(L, 3)) {
		val = (bool)lua_toboolean(L, 3);
	} else if (lua_istable(L, 3)) {
		int len = lua_objlen(L, 3);
		if (len == 2) {
			val = read_v2f(L, 3);
		} else if (len == 3) {
			val = read_v3f(L, 3);
		} else {
			// Try named fields x, y, z
			lua_getfield(L, 3, "x");
			lua_getfield(L, 3, "y");
			lua_getfield(L, 3, "z");
			if (!lua_isnil(L, -3) && !lua_isnil(L, -2)) {
				if (!lua_isnil(L, -1)) {
					val = v3f(lua_tonumber(L, -3), lua_tonumber(L, -2), lua_tonumber(L, -1));
				} else {
					val = v2f(lua_tonumber(L, -3), lua_tonumber(L, -2));
				}
			}
			lua_pop(L, 3);
		}
	} else {
		return 0;
	}

#if CHECK_CLIENT_BUILD()
	if (getScriptApiBase(L)->getType() == ScriptingType::Client) {
		IWritableShaderSource *shdsrc = getClient(L)->getShaderSource();
		if (shdsrc)
			shdsrc->setShaderUniform(shader_name, uniform_name, val);
	} else
#endif
	{
		getServer(L)->setShaderUniform(shader_name, uniform_name, val);
	}

	return 0;
}

// core.get_shader_names()
int ModApiShader::l_get_shader_names(lua_State *L)
{
	std::vector<std::string> targets = {
		"nodes_shader", "object_shader", "selection_shader", "clouds", "sky",
		"wieldmesh_shader", "second_stage", "extract_bloom", "volumetric_light",
		"bloom_downsample", "bloom_upsample", "update_exposure", "fxaa"
	};

	lua_newtable(L);
	for (size_t i = 0; i < targets.size(); i++) {
		lua_pushstring(L, targets[i].c_str());
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
