// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "lua_api/l_base.h"

class ModApiWorker : public ModApiBase
{
public:
	static void Initialize(lua_State *L, int top);
private:
	// create(name)
	static int l_create(lua_State *L);
	// queue(worker_id, func, args, mod_origin)
	static int l_queue(lua_State *L);
};
