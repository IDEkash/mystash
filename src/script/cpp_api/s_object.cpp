// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "s_object.h"
#include "common/c_content.h"
#include "common/c_converter.h"
#include "lua_api/l_object.h"

void ScriptApiObject::sceneobject_step(u16 id, float dtime)
{
	SCRIPTAPI_PRECHECKHEADER

	push_objectRef(L, id);
	lua_getfield(L, -1, "fire");
	if (lua_isfunction(L, -1)) {
		lua_pushvalue(L, -2); // self
		lua_pushstring(L, "update");
		lua_pushnumber(L, dtime);

		if (lua_pcall(L, 3, 0, error_handler))
			script_error(L, "error: %s", lua_tostring(L, -1));
	} else {
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
}

void ScriptApiObject::sceneobject_on_punch(u16 id, ServerActiveObject *puncher,
		float time_from_last_punch, const ToolCapabilities &toolcap, v3f dir)
{
	SCRIPTAPI_PRECHECKHEADER

	push_objectRef(L, id);
	lua_getfield(L, -1, "fire");
	if (lua_isfunction(L, -1)) {
		lua_pushvalue(L, -2); // self
		lua_pushstring(L, "hit");

		if (puncher)
			objectrefGetOrCreate(L, puncher);
		else
			lua_pushnil(L);

		if (lua_pcall(L, 3, 0, error_handler))
			script_error(L, "error: %s", lua_tostring(L, -1));
	} else {
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
}

void ScriptApiObject::sceneobject_on_rightclick(u16 id, ServerActiveObject *clicker)
{
	SCRIPTAPI_PRECHECKHEADER

	push_objectRef(L, id);
	lua_getfield(L, -1, "fire");
	if (lua_isfunction(L, -1)) {
		lua_pushvalue(L, -2); // self
		lua_pushstring(L, "interact");
		if (clicker)
			objectrefGetOrCreate(L, clicker);
		else
			lua_pushnil(L);

		if (lua_pcall(L, 3, 0, error_handler))
			script_error(L, "error: %s", lua_tostring(L, -1));
	} else {
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
}
