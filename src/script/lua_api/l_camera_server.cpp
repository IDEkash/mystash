// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "l_camera_server.h"
#include "l_internal.h"
#include "script/common/c_converter.h"
#include "script/common/c_content.h"
#include "server.h"
#include "remoteplayer.h"
#include "server/player_sao.h"

int ModApiCamera::l_get_rotation(lua_State *L)
{
	RemotePlayer *player = getplayer(checkObject<ObjectRef>(L, 1));
	if (!player) return 0;

	lua_newtable(L);
	lua_pushnumber(L, player->m_camera_state.yaw);
	lua_setfield(L, -2, "yaw");
	lua_pushnumber(L, player->m_camera_state.pitch);
	lua_setfield(L, -2, "pitch");
	return 1;
}

int ModApiCamera::l_set_rotation(lua_State *L)
{
	RemotePlayer *player = getplayer(checkObject<ObjectRef>(L, 1));
	if (!player) return 0;

	if (lua_istable(L, 2)) {
		lua_getfield(L, 2, "yaw");
		if (lua_isnumber(L, -1)) player->m_camera_state.yaw = lua_tonumber(L, -1);
		lua_pop(L, 1);
		lua_getfield(L, 2, "pitch");
		if (lua_isnumber(L, -1)) player->m_camera_state.pitch = lua_tonumber(L, -1);
		lua_pop(L, 1);
	}

	getServer(L)->SendSetCamera(player->getPeerId(), 0x02);
	return 0;
}

int ModApiCamera::l_add_rotation(lua_State *L)
{
	RemotePlayer *player = getplayer(checkObject<ObjectRef>(L, 1));
	if (!player) return 0;

	if (lua_istable(L, 2)) {
		lua_getfield(L, 2, "yaw");
		if (lua_isnumber(L, -1)) player->m_camera_state.yaw += lua_tonumber(L, -1);
		lua_pop(L, 1);
		lua_getfield(L, 2, "pitch");
		if (lua_isnumber(L, -1)) player->m_camera_state.pitch += lua_tonumber(L, -1);
		lua_pop(L, 1);
	}

	getServer(L)->SendSetCamera(player->getPeerId(), 0x02);
	return 0;
}

int ModApiCamera::l_lerp_rotation(lua_State *L)
{
	RemotePlayer *player = getplayer(checkObject<ObjectRef>(L, 1));
	if (!player) return 0;

	if (lua_istable(L, 2)) {
		lua_getfield(L, 2, "yaw");
		if (lua_isnumber(L, -1)) player->m_camera_state.yaw = lua_tonumber(L, -1);
		lua_pop(L, 1);
		lua_getfield(L, 2, "pitch");
		if (lua_isnumber(L, -1)) player->m_camera_state.pitch = lua_tonumber(L, -1);
		lua_pop(L, 1);
	}
	f32 duration = luaL_checknumber(L, 3);

	getServer(L)->SendSetCamera(player->getPeerId(), 0x02, duration);
	return 0;
}

int ModApiCamera::l_reset(lua_State *L)
{
	RemotePlayer *player = getplayer(checkObject<ObjectRef>(L, 1));
	if (!player) return 0;

	player->m_camera_state.yaw = 0;
	player->m_camera_state.pitch = 0;
	player->m_camera_state.fov = 0;
	getServer(L)->SendSetCamera(player->getPeerId(), 0x02 | 0x04);
	return 0;
}

int ModApiCamera::l_get_fov(lua_State *L)
{
	RemotePlayer *player = getplayer(checkObject<ObjectRef>(L, 1));
	if (!player) return 0;
	lua_pushnumber(L, player->m_camera_state.fov);
	return 1;
}

int ModApiCamera::l_set_fov(lua_State *L)
{
	RemotePlayer *player = getplayer(checkObject<ObjectRef>(L, 1));
	if (!player) return 0;
	player->m_camera_state.fov = luaL_checknumber(L, 2);
	getServer(L)->SendSetCamera(player->getPeerId(), 0x04);
	return 0;
}

int ModApiCamera::l_lerp_fov(lua_State *L)
{
	RemotePlayer *player = getplayer(checkObject<ObjectRef>(L, 1));
	if (!player) return 0;
	player->m_camera_state.fov = luaL_checknumber(L, 2);
	f32 duration = luaL_checknumber(L, 3);
	getServer(L)->SendSetCamera(player->getPeerId(), 0x04, duration);
	return 0;
}

int ModApiCamera::l_get_mode(lua_State *L)
{
	RemotePlayer *player = getplayer(checkObject<ObjectRef>(L, 1));
	if (!player) return 0;
	CameraMode mode = player->m_camera_state.mode;
	switch (mode) {
	case CAMERA_MODE_FIRST: lua_pushstring(L, "firstperson"); break;
	case CAMERA_MODE_THIRD: lua_pushstring(L, "thirdpersonback"); break;
	case CAMERA_MODE_THIRD_FRONT: lua_pushstring(L, "thirdpersonfront"); break;
	case CAMERA_MODE_SPECTATE: lua_pushstring(L, "spectate"); break;
	default: lua_pushstring(L, enum_to_string(es_CameraMode, mode)); break;
	}
	return 1;
}

int ModApiCamera::l_set_mode(lua_State *L)
{
	RemotePlayer *player = getplayer(checkObject<ObjectRef>(L, 1));
	if (!player) return 0;

	CameraMode mode;
	if (lua_isnumber(L, 2)) {
		mode = (CameraMode)((int)lua_tonumber(L, 2));
	} else {
		std::string s = luaL_checkstring(L, 2);
		if (s == "firstperson") mode = CAMERA_MODE_FIRST;
		else if (s == "thirdpersonback") mode = CAMERA_MODE_THIRD;
		else if (s == "thirdpersonfront") mode = CAMERA_MODE_THIRD_FRONT;
		else if (s == "spectate") mode = CAMERA_MODE_SPECTATE;
		else return 0;
	}
	player->m_camera_state.mode = mode;
	getServer(L)->SendSetCamera(player->getPeerId(), 0x08);
	return 0;
}

int ModApiCamera::l_get_position(lua_State *L)
{
	RemotePlayer *player = getplayer(checkObject<ObjectRef>(L, 1));
	if (!player) return 0;
	push_v3f(L, player->m_camera_state.pos / BS);
	return 1;
}

int ModApiCamera::l_set_position(lua_State *L)
{
	RemotePlayer *player = getplayer(checkObject<ObjectRef>(L, 1));
	if (!player) return 0;
	player->m_camera_state.pos = read_v3f(L, 2) * BS;
	getServer(L)->SendSetCamera(player->getPeerId(), 0x01);
	return 0;
}

int ModApiCamera::l_add_position(lua_State *L)
{
	RemotePlayer *player = getplayer(checkObject<ObjectRef>(L, 1));
	if (!player) return 0;
	player->m_camera_state.pos += read_v3f(L, 2) * BS;
	getServer(L)->SendSetCamera(player->getPeerId(), 0x01);
	return 0;
}

int ModApiCamera::l_lerp_position(lua_State *L)
{
	RemotePlayer *player = getplayer(checkObject<ObjectRef>(L, 1));
	if (!player) return 0;
	player->m_camera_state.pos = read_v3f(L, 2) * BS;
	f32 duration = luaL_checknumber(L, 3);
	getServer(L)->SendSetCamera(player->getPeerId(), 0x01, duration);
	return 0;
}

int ModApiCamera::l_reset_position(lua_State *L)
{
	RemotePlayer *player = getplayer(checkObject<ObjectRef>(L, 1));
	if (!player) return 0;
	player->m_camera_state.pos = v3f(NAN, NAN, NAN);
	getServer(L)->SendSetCamera(player->getPeerId(), 0x01);
	return 0;
}

void ModApiCamera::Initialize(lua_State *L, int top)
{
	lua_newtable(L);
	int tbl = lua_gettop(L);

	registerFunction(L, "get_rotation", l_get_rotation, tbl);
	registerFunction(L, "set_rotation", l_set_rotation, tbl);
	registerFunction(L, "add_rotation", l_add_rotation, tbl);
	registerFunction(L, "lerp_rotation", l_lerp_rotation, tbl);
	registerFunction(L, "reset", l_reset, tbl);

	registerFunction(L, "get_fov", l_get_fov, tbl);
	registerFunction(L, "set_fov", l_set_fov, tbl);
	registerFunction(L, "lerp_fov", l_lerp_fov, tbl);

	registerFunction(L, "get_mode", l_get_mode, tbl);
	registerFunction(L, "set_mode", l_set_mode, tbl);
	registerFunction(L, "setmode", l_set_mode, tbl);

	registerFunction(L, "get_position", l_get_position, tbl);
	registerFunction(L, "set_position", l_set_position, tbl);
	registerFunction(L, "add_position", l_add_position, tbl);
	registerFunction(L, "lerp_position", l_lerp_position, tbl);
	registerFunction(L, "reset_position", l_reset_position, tbl);

	lua_setfield(L, top, "camera");
}
