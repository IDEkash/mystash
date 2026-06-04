// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "l_worker.h"
#include "lua_api/l_internal.h"
#include "scripting_server.h"
#include "cpp_api/s_worker.h"
#include "common/c_packer.h"

int ModApiWorker::l_create(lua_State *L)
{
	std::string name = luaL_checkstring(L, 1);
	ServerScripting *script = (ServerScripting *)getScriptApiBase(L);
	u32 id = script->createWorker(name);
	lua_pushinteger(L, id);
	return 1;
}

int ModApiWorker::l_queue(lua_State *L)
{
	u32 worker_id = luaL_checkinteger(L, 1);
	luaL_checktype(L, 2, LUA_TSTRING); // serialized func
	luaL_checktype(L, 3, LUA_TTABLE); // args table
	std::string mod_origin = luaL_checkstring(L, 4);

	size_t func_len;
	const char *func_ptr = lua_tolstring(L, 2, &func_len);
	std::string serialized_func(func_ptr, func_len);

	PackedValue *args = script_pack(L, 3);

	ServerScripting *script = (ServerScripting *)getScriptApiBase(L);
	u32 job_id = script->queueWorkerJob(worker_id, std::move(serialized_func), args, mod_origin);

	lua_pushinteger(L, job_id);
	return 1;
}

void ModApiWorker::Initialize(lua_State *L, int top)
{
	lua_newtable(L);
	int worker_top = lua_gettop(L);

	lua_pushcfunction(L, l_create);
	lua_setfield(L, worker_top, "_create");

	lua_pushcfunction(L, l_queue);
	lua_setfield(L, worker_top, "_queue");

	lua_setfield(L, top, "worker");
}
