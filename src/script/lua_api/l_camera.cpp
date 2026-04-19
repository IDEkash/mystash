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
	LocalPlayer *player = getClient(L)->getEnv().getLocalPlayer();
	GenericCAO *playercao = player->getCAO();
	if (!camera)
		return 0;
	sanity_check(playercao);

	CameraMode mode;
	if (lua_isnumber(L, 2)) {
		mode = (CameraMode)((int)lua_tonumber(L, 2));
	} else if (lua_isstring(L, 2)) {
		std::string s = lua_tostring(L, 2);
		if (s == "firstperson") mode = CAMERA_MODE_FIRST;
		else if (s == "thirdpersonback") mode = CAMERA_MODE_THIRD;
		else if (s == "thirdpersonfront") mode = CAMERA_MODE_THIRD_FRONT;
		else if (s == "spectate") mode = CAMERA_MODE_SPECTATE;
		else return 0;
	} else {
		return 0;
	}

	CameraMode old_mode = camera->getCameraMode();
	if (mode == old_mode)
		return 0;

	// Transition offsets/positions
	if (mode == CAMERA_MODE_SPECTATE) {
		// Entering spectate: convert relative to absolute
		camera->setLuaPos(player->getEyePosition());
		camera->setLuaYaw(player->getYaw() + camera->getLuaYaw());
		camera->setLuaPitch(player->getPitch() + camera->getLuaPitch());
	} else if (old_mode == CAMERA_MODE_SPECTATE) {
		// Leaving spectate: convert absolute back to relative
		camera->setLuaYaw(camera->getLuaYaw() - player->getYaw());
		camera->setLuaPitch(camera->getLuaPitch() - player->getPitch());
	}

	camera->setCameraMode(mode);
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

// get_mode(self)
int LuaCamera::l_get_mode(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	CameraMode mode = camera->getCameraMode();
	switch (mode) {
	case CAMERA_MODE_FIRST: lua_pushstring(L, "firstperson"); break;
	case CAMERA_MODE_THIRD: lua_pushstring(L, "thirdpersonback"); break;
	case CAMERA_MODE_THIRD_FRONT: lua_pushstring(L, "thirdpersonfront"); break;
	case CAMERA_MODE_SPECTATE: lua_pushstring(L, "spectate"); break;
	default: lua_pushinteger(L, (int)mode); break;
	}

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

// set_fov(self, fov)
int LuaCamera::l_set_fov(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	f32 fov = luaL_checknumber(L, 2);
	camera->lerpFov(fov, 0.0f);
	return 0;
}

// lerp_fov(self, fov, duration)
int LuaCamera::l_lerp_fov(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	f32 fov = luaL_checknumber(L, 2);
	f32 duration = luaL_checknumber(L, 3);
	camera->lerpFov(fov, duration);
	return 0;
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

// get_rotation(self)
int LuaCamera::l_get_rotation(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	lua_newtable(L);
	lua_pushnumber(L, camera->getYaw());
	lua_setfield(L, -2, "yaw");
	lua_pushnumber(L, camera->getPitch());
	lua_setfield(L, -2, "pitch");
	return 1;
}

// set_rotation(self, {yaw, pitch})
int LuaCamera::l_set_rotation(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	f32 target_yaw = camera->getYaw();
	f32 target_pitch = camera->getPitch();

	if (lua_istable(L, 2)) {
		lua_getfield(L, 2, "yaw");
		if (lua_isnumber(L, -1)) target_yaw = lua_tonumber(L, -1);
		lua_pop(L, 1);
		lua_getfield(L, 2, "pitch");
		if (lua_isnumber(L, -1)) target_pitch = lua_tonumber(L, -1);
		lua_pop(L, 1);
	}

	if (camera->getCameraMode() != CAMERA_MODE_SPECTATE) {
		LocalPlayer *player = getClient(L)->getEnv().getLocalPlayer();
		target_yaw -= player->getYaw();
		target_pitch -= player->getPitch();
	}

	camera->lerpRotation(target_yaw, target_pitch, 0.0f);
	return 0;
}

// add_rotation(self, {yaw, pitch})
int LuaCamera::l_add_rotation(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	f32 yaw = 0;
	f32 pitch = 0;

	if (lua_istable(L, 2)) {
		lua_getfield(L, 2, "yaw");
		if (lua_isnumber(L, -1)) yaw = lua_tonumber(L, -1);
		lua_pop(L, 1);
		lua_getfield(L, 2, "pitch");
		if (lua_isnumber(L, -1)) pitch = lua_tonumber(L, -1);
		lua_pop(L, 1);
	}

	camera->lerpRotation(camera->getLuaYaw() + yaw, camera->getLuaPitch() + pitch, 0.0f, true);
	return 0;
}

// lerp_rotation(self, {yaw, pitch}, duration)
int LuaCamera::l_lerp_rotation(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	f32 target_yaw = camera->getYaw();
	f32 target_pitch = camera->getPitch();

	if (lua_istable(L, 2)) {
		lua_getfield(L, 2, "yaw");
		if (lua_isnumber(L, -1)) target_yaw = lua_tonumber(L, -1);
		lua_pop(L, 1);
		lua_getfield(L, 2, "pitch");
		if (lua_isnumber(L, -1)) target_pitch = lua_tonumber(L, -1);
		lua_pop(L, 1);
	}

	if (camera->getCameraMode() != CAMERA_MODE_SPECTATE) {
		LocalPlayer *player = getClient(L)->getEnv().getLocalPlayer();
		target_yaw -= player->getYaw();
		target_pitch -= player->getPitch();
	}

	f32 duration = luaL_checknumber(L, 3);
	camera->lerpRotation(target_yaw, target_pitch, duration);
	return 0;
}

// reset(self)
int LuaCamera::l_reset(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	camera->lerpRotation(0, 0, 0.0f);
	camera->resetLuaFov();
	return 0;
}

// get_position(self)
int LuaCamera::l_get_position(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	push_v3f(L, camera->getLuaPos() / BS);
	return 1;
}

// set_position(self, pos)
int LuaCamera::l_set_position(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	v3f pos = read_v3f(L, 2) * BS;
	camera->lerpPos(pos, 0.0f);
	return 0;
}

// add_position(self, pos)
int LuaCamera::l_add_position(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	v3f pos = read_v3f(L, 2) * BS;
	camera->lerpPos(camera->getLuaPos() + pos, 0.0f);
	return 0;
}

// lerp_position(self, pos, duration)
int LuaCamera::l_lerp_position(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	v3f pos = read_v3f(L, 2) * BS;
	f32 duration = luaL_checknumber(L, 3);
	camera->lerpPos(pos, duration);
	return 0;
}

// reset_position(self)
int LuaCamera::l_reset_position(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	LocalPlayer *player = getClient(L)->getEnv().getLocalPlayer();
	camera->lerpPos(player->getEyePosition(), 0.0f);
	return 0;
}

Camera *LuaCamera::getobject(LuaCamera *ref)
{
	return ref->m_camera;
}

Camera *LuaCamera::getobject(lua_State *L, int narg)
{
	if (lua_type(L, narg) == LUA_TUSERDATA) {
		void *ud = lua_touserdata(L, narg);
		if (lua_getmetatable(L, narg)) {
			luaL_getmetatable(L, className);
			bool is_camera = lua_rawequal(L, -1, -2);
			lua_pop(L, 2);
			if (is_camera) {
				LuaCamera *ref = *(LuaCamera **)ud;
				return getobject(ref);
			}
		}
	}

	// For core.camera.fn(player, ...) pattern
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "camera");
	if (lua_type(L, -1) == LUA_TUSERDATA) {
		void *ud = lua_touserdata(L, -1);
		lua_pop(L, 2); // pop core, camera
		LuaCamera *ref = *(LuaCamera **)ud;
		return getobject(ref);
	}
	lua_pop(L, 2);

	return nullptr;
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

	// Add aliases
	luaL_getmetatable(L, className);
	int metatable = lua_gettop(L);
	lua_getfield(L, metatable, "set_camera_mode");
	lua_setfield(L, metatable, "set_mode");
	lua_getfield(L, metatable, "set_camera_mode");
	lua_setfield(L, metatable, "setmode");
	lua_pop(L, 1);
}

const char LuaCamera::className[] = "Camera";
const luaL_Reg LuaCamera::methods[] = {
	luamethod(LuaCamera, set_camera_mode),
	luamethod(LuaCamera, set_camera_mode), // set_mode alias added below
	luamethod(LuaCamera, get_camera_mode),
	luamethod(LuaCamera, get_mode),
	luamethod(LuaCamera, get_fov),
	luamethod(LuaCamera, set_fov),
	luamethod(LuaCamera, lerp_fov),
	luamethod(LuaCamera, get_pos),
	luamethod(LuaCamera, get_offset),
	luamethod(LuaCamera, get_look_dir),
	luamethod(LuaCamera, get_look_vertical),
	luamethod(LuaCamera, get_look_horizontal),
	luamethod(LuaCamera, get_aspect_ratio),

	luamethod(LuaCamera, get_rotation),
	luamethod(LuaCamera, set_rotation),
	luamethod(LuaCamera, add_rotation),
	luamethod(LuaCamera, lerp_rotation),
	luamethod(LuaCamera, reset),

	luamethod(LuaCamera, get_position),
	luamethod(LuaCamera, set_position),
	luamethod(LuaCamera, add_position),
	luamethod(LuaCamera, lerp_position),
	luamethod(LuaCamera, reset_position),

	{0, 0}
};
