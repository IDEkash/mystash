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
#include <optional>

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
	GenericCAO *playercao = getClient(L)->getEnv().getLocalPlayer()->getCAO();
	if (!camera)
		return 0;
	sanity_check(playercao);
	if (!lua_isnumber(L, 2))
		return 0;

	camera->setCameraMode((CameraMode)((int)lua_tonumber(L, 2)));
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

// set_override(self, override_table)
int LuaCamera::l_set_override(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	CameraOverride ov;
	if (lua_istable(L, 2)) {
		lua_getfield(L, 2, "wield_offset");
		if (lua_istable(L, -1))
			ov.wield_offset = read_v2f(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, 2, "wield_rotation");
		if (lua_istable(L, -1))
			ov.wield_rotation = read_v3f(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, 2, "wield_fov");
		if (lua_isnumber(L, -1))
			ov.wield_fov = lua_tonumber(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, 2, "bob_amount");
		if (lua_isnumber(L, -1))
			ov.bob_amount = lua_tonumber(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, 2, "bob_speed");
		if (lua_isnumber(L, -1))
			ov.bob_speed = lua_tonumber(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, 2, "roll");
		if (lua_isnumber(L, -1))
			ov.roll = lua_tonumber(L, -1) * core::DEGTORAD;
		lua_pop(L, 1);

		lua_getfield(L, 2, "pos_offset");
		if (lua_istable(L, -1))
			ov.pos_offset = read_v3f(L, -1) * BS;
		lua_pop(L, 1);
	}

	camera->setOverride(ov);
	return 0;
}

// add_trauma(self, amount, [config])
int LuaCamera::l_add_trauma(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	f32 amount = luaL_checknumber(L, 2);
	std::optional<f32> decay;
	std::optional<f32> max_angle;
	std::optional<f32> max_offset;

	if (lua_istable(L, 3)) {
		lua_getfield(L, 3, "decay");
		if (lua_isnumber(L, -1))
			decay = lua_tonumber(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, 3, "max_angle");
		if (lua_isnumber(L, -1))
			max_angle = lua_tonumber(L, -1) * core::DEGTORAD;
		lua_pop(L, 1);

		lua_getfield(L, 3, "max_offset");
		if (lua_isnumber(L, -1))
			max_offset = lua_tonumber(L, -1) * BS;
		lua_pop(L, 1);
	}

	camera->addTrauma(amount, decay, max_angle, max_offset);
	return 0;
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
	luamethod(LuaCamera, set_override),
	luamethod(LuaCamera, add_trauma),

	{0, 0}
};
