// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 sapier
// Copyright (C) 2025 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include "lua_api/l_base.h"

class ModApiContent : public ModApiBase {
public:
	static void Initialize(lua_State *L, int top);
	static void InitializeAsync(lua_State *L, int top);

private:
	// get_worlds() -> {worldspec, ...}
	static int l_get_worlds(lua_State *L);

	// get_games() -> {gamespec, ...}
	static int l_get_games(lua_State *L);

	// get_mapgen_names([include_hidden]) -> {name, ...}
	static int l_get_mapgen_names(lua_State *L);

	// create_world(def) -> success, error_message
	static int l_create_world(lua_State *L);

	// delete_world(world_name) -> success, error_message
	static int l_delete_world(lua_State *L);

	// get_game_settings(game_id) -> {setting, ...}, error_message
	static int l_get_game_settings(lua_State *L);
};
