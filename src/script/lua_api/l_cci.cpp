#include "script/lua_api/l_cci.h"
#include "common/c_converter.h"
#include "lua_api/l_internal.h"
#include "server.h"
#include "remoteplayer.h"
#include "serverenvironment.h"

int ModApiCCI::l_cci_register_style(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string style_name = readParam<std::string>(L, 1);
	luaL_checktype(L, 2, LUA_TTABLE);

	CCIStyle style;
	style.name = style_name;

	// 1. Read geometry table
	lua_getfield(L, 2, "geometry");
	if (lua_istable(L, -1)) {
		int geom_idx = lua_gettop(L);

		// 1.1. Read points
		lua_getfield(L, geom_idx, "points");
		if (lua_istable(L, -1)) {
			int points_idx = lua_gettop(L);
			lua_pushnil(L);
			while (lua_next(L, points_idx) != 0) {
				// key at -2, value at -1
				if (lua_type(L, -2) == LUA_TSTRING) {
					std::string p_name = lua_tostring(L, -2);
					v2f p_pos = read_v2f(L, -1);
					style.points[p_name] = p_pos;
				}
				lua_pop(L, 1);
			}
		}
		lua_pop(L, 1);

		// 1.2. Read shape connection array
		lua_getfield(L, geom_idx, "shape");
		if (lua_istable(L, -1)) {
			int shape_idx = lua_gettop(L);
			int len = lua_objlen(L, shape_idx);
			for (int i = 1; i <= len; ++i) {
				lua_rawgeti(L, shape_idx, i);
				if (lua_istable(L, -1)) {
					CCIConnection conn;
					// [1]: p1 (string)
					lua_rawgeti(L, -1, 1);
					if (lua_isstring(L, -1))
						conn.p1 = lua_tostring(L, -1);
					lua_pop(L, 1);

					// [2]: p2 (string)
					lua_rawgeti(L, -1, 2);
					if (lua_isstring(L, -1))
						conn.p2 = lua_tostring(L, -1);
					lua_pop(L, 1);

					// [3]: bend (float, optional)
					lua_rawgeti(L, -1, 3);
					if (lua_isnumber(L, -1))
						conn.bend = (float)lua_tonumber(L, -1);
					else
						conn.bend = 0.0f;
					lua_pop(L, 1);

					if (!conn.p1.empty() && !conn.p2.empty()) {
						style.shape.push_back(conn);
					}
				}
				lua_pop(L, 1);
			}
		}
		lua_pop(L, 1);
	}
	lua_pop(L, 1);

	// 2. Read appearance table
	lua_getfield(L, 2, "appearance");
	if (lua_istable(L, -1)) {
		int app_idx = lua_gettop(L);

		// 2.1. Read fill color
		lua_getfield(L, app_idx, "fill");
		if (!lua_isnil(L, -1)) {
			style.has_fill = read_color(L, -1, &style.fill_color);
		}
		lua_pop(L, 1);

		// 2.2. Read opacity
		style.opacity = getfloatfield_default(L, app_idx, "opacity", 1.0f);

		// 2.3. Read image table
		lua_getfield(L, app_idx, "image");
		if (lua_istable(L, -1)) {
			int img_idx = lua_gettop(L);
			style.has_image = true;

			// [1]: texture path (string)
			lua_rawgeti(L, img_idx, 1);
			if (style.has_image && lua_isstring(L, -1))
				style.image.texture = lua_tostring(L, -1);
			lua_pop(L, 1);

			// position (v2f, optional)
			lua_getfield(L, img_idx, "position");
			if (!lua_isnil(L, -1))
				style.image.position = read_v2f(L, -1);
			lua_pop(L, 1);

			// size (float, optional)
			style.image.size = getfloatfield_default(L, img_idx, "size", 1.0f);
		}
		lua_pop(L, 1);
	}
	lua_pop(L, 1);

	// Register style on server CCIManager
	getServer(L)->getCCIManager()->registerStyle(style);

	// Broadcast the style to all connected clients
	getServer(L)->SendCCIStyle(PEER_ID_INEXISTENT, style);

	return 0;
}

int ModApiCCI::l_cci_create_instance(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string instance_name = readParam<std::string>(L, 1);
	std::string style_name = readParam<std::string>(L, 2);
	luaL_checktype(L, 3, LUA_TTABLE);

	CCIInstance instance;
	instance.name = instance_name;
	instance.style_name = style_name;

	// Read position
	lua_getfield(L, 3, "position");
	if (lua_istable(L, -1)) {
		instance.position = read_v2f(L, -1);
	}
	lua_pop(L, 1);

	// Read layer
	instance.layer = getintfield_default(L, 3, "layer", 1);

	// Read player name
	lua_getfield(L, 3, "player");
	if (lua_isstring(L, -1)) {
		instance.player = lua_tostring(L, -1);
	}
	lua_pop(L, 1);

	// Register instance on server CCIManager
	getServer(L)->getCCIManager()->registerInstance(instance);

	// Send instance creation to client(s)
	if (instance.player.empty()) {
		getServer(L)->SendCCICreate(PEER_ID_INEXISTENT, instance);
	} else {
		// Look up player's peer ID and send
		RemotePlayer *player = getServer(L)->getEnv().getPlayer(instance.player.c_str());
		if (player) {
			getServer(L)->SendCCICreate(player->getPeerId(), instance);
		}
	}

	return 0;
}

int ModApiCCI::l_cci_destroy_instance(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string instance_name = readParam<std::string>(L, 1);
	std::string player_name = "";
	if (!lua_isnoneornil(L, 2)) {
		player_name = readParam<std::string>(L, 2);
	}

	// Remove from server CCIManager
	getServer(L)->getCCIManager()->destroyInstance(instance_name);

	// Send destroy to client(s)
	if (player_name.empty()) {
		getServer(L)->SendCCIDestroy(PEER_ID_INEXISTENT, instance_name);
	} else {
		RemotePlayer *player = getServer(L)->getEnv().getPlayer(player_name.c_str());
		if (player) {
			getServer(L)->SendCCIDestroy(player->getPeerId(), instance_name);
		}
	}

	return 0;
}

void ModApiCCI::Initialize(lua_State *L, int top)
{
	registerFunction(L, "cci_register_style", l_cci_register_style, top);
	registerFunction(L, "cci_create_instance", l_cci_create_instance, top);
	registerFunction(L, "cci_destroy_instance", l_cci_destroy_instance, top);
}
