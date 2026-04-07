// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

struct lua_State;
class RegistryEntry;

class LuaBridge {
public:
	static void build_table(lua_State *L, RegistryEntry *entry);
};
