// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "l_camera.h"
#include "script/common/c_converter.h"
#include "l_internal.h"
#include "client/content_cao.h"
#include "client/camera.h"
#include "client/client.h"
#include "client/localplayer.h"
#include <ICameraSceneNode.h>

LuaCamera::LuaCamera(Camera *m) : m_camera(m)
{
}

void LuaCamera::create(lua_State *L, Camera *m)
{
	lua_getglobal(L, "core");
	luaL_checktype(L, -1, LUA_TTABLE);
	int objectstable = lua_gettop(L);
	lua_getfield(L, -1, "camera");

	// Duplication check
	if (lua_type(L, -1) == LUA_TUSERDATA) {
		lua_pop(L, 1);
		return;
	}

	LuaCamera *o = new LuaCamera(m);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);

	lua_pushvalue(L, lua_gettop(L));
	lua_setfield(L, objectstable, "camera");
}

// set_camera_mode(self, mode)
int LuaCamera::l_set_camera_mode(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	LocalPlayer *player = getClient(L)->getEnv().getLocalPlayer();
	if (!player)
		return 0;

	GenericCAO *playercao = player->getCAO();
	if (!playercao)
		return 0;

	if (!lua_isnumber(L, 2))
		return 0;

	int mode_int = (int)lua_tonumber(L, 2);
	if (mode_int < CAMERA_MODE_FIRST || mode_int > CAMERA_MODE_THIRD_FRONT)
		return 0;

	camera->setCameraMode((CameraMode)mode_int);
	// Make the player visible depending on camera mode.
	playercao->updateMeshCulling();
	playercao->setChildrenVisible(camera->getCameraMode() > CAMERA_MODE_FIRST);
	return 0;
}

// get_camera_mode(self)
int LuaCamera::l_get_camera_mode(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	lua_pushinteger(L, (int)camera->getCameraMode());

	return 1;
}

// get_fov(self)
int LuaCamera::l_get_fov(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	lua_newtable(L);
	lua_pushnumber(L, camera->getFovX() * core::RADTODEG);
	lua_setfield(L, -2, "x");
	lua_pushnumber(L, camera->getFovY() * core::RADTODEG);
	lua_setfield(L, -2, "y");
	lua_pushnumber(L, camera->getCameraNode()->getFOV() * core::RADTODEG);
	lua_setfield(L, -2, "actual");
	lua_pushnumber(L, camera->getFovMax() * core::RADTODEG);
	lua_setfield(L, -2, "max");
	return 1;
}

// get_pos(self)
int LuaCamera::l_get_pos(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	push_v3f(L, camera->getPosition() / BS);
	return 1;
}

// get_offset(self)
int LuaCamera::l_get_offset(lua_State *L)
{
	LocalPlayer *player = getClient(L)->getEnv().getLocalPlayer();
	sanity_check(player);

	push_v3f(L, player->getEyeOffset() / BS);
	return 1;
}

// get_look_dir(self)
int LuaCamera::l_get_look_dir(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	push_v3f(L, camera->getDirection());
	return 1;
}

// get_look_horizontal(self)
// FIXME: wouldn't localplayer be a better place for this?
int LuaCamera::l_get_look_horizontal(lua_State *L)
{
	LocalPlayer *player = getClient(L)->getEnv().getLocalPlayer();
	sanity_check(player);

	lua_pushnumber(L, (player->getYaw() + 90.f) * core::DEGTORAD);
	return 1;
}

// get_look_vertical(self)
// FIXME: wouldn't localplayer be a better place for this?
int LuaCamera::l_get_look_vertical(lua_State *L)
{
	LocalPlayer *player = getClient(L)->getEnv().getLocalPlayer();
	sanity_check(player);

	lua_pushnumber(L, -1.0f * player->getPitch() * core::DEGTORAD);
	return 1;
}

// get_aspect_ratio(self)
int LuaCamera::l_get_aspect_ratio(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	lua_pushnumber(L, camera->getCameraNode()->getAspectRatio());
	return 1;
}

static void push_camera_modifier_csm(lua_State *L, const CameraModifier &mod) {
	lua_createtable(L, 0, 6);
	push_v3f(L, mod.offset);
	lua_setfield(L, -2, "offset");
	push_v3f(L, mod.rotation);
	lua_setfield(L, -2, "rotation");
	lua_pushnumber(L, mod.fov);
	lua_setfield(L, -2, "fov");

	lua_createtable(L, 0, 2);
	lua_pushnumber(L, mod.shake_intensity);
	lua_setfield(L, -2, "intensity");
	lua_pushnumber(L, mod.shake_speed);
	lua_setfield(L, -2, "speed");
	lua_setfield(L, -2, "shake");

	push_v3f(L, mod.recoil);
	lua_setfield(L, -2, "recoil");

	lua_createtable(L, 0, 2);
	lua_pushnumber(L, mod.sway_intensity);
	lua_setfield(L, -2, "intensity");
	lua_pushnumber(L, mod.sway_speed);
	lua_setfield(L, -2, "speed");
	lua_setfield(L, -2, "sway");
}

int LuaCamera::l_set_modifier(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	std::string name = readParam<std::string>(L, 2);

	if (lua_isnoneornil(L, 3)) {
		camera->removeModifier(name);
	} else {
		luaL_checktype(L, 3, LUA_TTABLE);
		CameraModifier mod;

		lua_getfield(L, 3, "offset");
		if (lua_istable(L, -1) || (lua_isuserdata(L, -1) && !lua_isnil(L, -1))) {
			mod.offset = read_v3f(L, -1);
		}
		lua_pop(L, 1);

		lua_getfield(L, 3, "rotation");
		if (lua_istable(L, -1) || (lua_isuserdata(L, -1) && !lua_isnil(L, -1))) {
			mod.rotation = read_v3f(L, -1);
		}
		lua_pop(L, 1);

		mod.fov = getfloatfield_default(L, 3, "fov", 0.0f);

		lua_getfield(L, 3, "shake");
		if (lua_istable(L, -1)) {
			mod.shake_intensity = getfloatfield_default(L, -1, "intensity", 0.0f);
			mod.shake_speed = getfloatfield_default(L, -1, "speed", 0.0f);
		}
		lua_pop(L, 1);

		lua_getfield(L, 3, "recoil");
		if (lua_istable(L, -1) || (lua_isuserdata(L, -1) && !lua_isnil(L, -1))) {
			mod.recoil = read_v3f(L, -1);
		}
		lua_pop(L, 1);

		lua_getfield(L, 3, "sway");
		if (lua_istable(L, -1)) {
			mod.sway_intensity = getfloatfield_default(L, -1, "intensity", 0.0f);
			mod.sway_speed = getfloatfield_default(L, -1, "speed", 0.0f);
		}
		lua_pop(L, 1);

		camera->setModifier(name, mod);
	}

	return 0;
}

int LuaCamera::l_get_modifier(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	std::string name = readParam<std::string>(L, 2);
	auto mods = camera->getModifiers();
	auto it = mods.find(name);
	if (it == mods.end()) {
		lua_pushnil(L);
		return 1;
	}

	push_camera_modifier_csm(L, it->second);
	return 1;
}

int LuaCamera::l_get_modifiers(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	auto mods = camera->getModifiers();
	lua_createtable(L, 0, mods.size());
	for (const auto &pair : mods) {
		push_camera_modifier_csm(L, pair.second);
		lua_setfield(L, -2, pair.first.c_str());
	}
	return 1;
}

Camera *LuaCamera::getobject(LuaCamera *ref)
{
	return ref->m_camera;
}

Camera *LuaCamera::getobject(lua_State *L, int narg)
{
	LuaCamera *ref = checkObject<LuaCamera>(L, narg);
	assert(ref);
	return getobject(ref);
}

int LuaCamera::gc_object(lua_State *L)
{
	LuaCamera *o = *(LuaCamera **)(lua_touserdata(L, 1));
	delete o;
	return 0;
}

void LuaCamera::Register(lua_State *L)
{
	static const luaL_Reg metamethods[] = {
		{"__gc", gc_object},
		{0, 0}
	};
	registerClass<LuaCamera>(L, methods, metamethods);
}

const char LuaCamera::className[] = "Camera";
const luaL_Reg LuaCamera::methods[] = {
	luamethod(LuaCamera, set_camera_mode),
	luamethod(LuaCamera, get_camera_mode),
	luamethod(LuaCamera, get_fov),
	luamethod(LuaCamera, get_pos),
	luamethod(LuaCamera, get_offset),
	luamethod(LuaCamera, get_look_dir),
	luamethod(LuaCamera, get_look_vertical),
	luamethod(LuaCamera, get_look_horizontal),
	luamethod(LuaCamera, get_aspect_ratio),
	luamethod(LuaCamera, set_modifier),
	luamethod(LuaCamera, get_modifier),
	luamethod(LuaCamera, get_modifiers),

	{0, 0}
};
