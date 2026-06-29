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

LuaCamera::~LuaCamera()
{
	if (m_raw_cam)
		m_raw_cam->remove();
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
	LuaCamera *ref = checkObject<LuaCamera>(L, 1);
	scene::ICameraSceneNode *cam = ref->m_camera ? ref->m_camera->getCameraNode() : ref->m_raw_cam;
	if (!cam)
		return 0;

	lua_newtable(L);
	if (ref->m_camera) {
		lua_pushnumber(L, ref->m_camera->getFovX() * core::RADTODEG);
		lua_setfield(L, -2, "x");
		lua_pushnumber(L, ref->m_camera->getFovY() * core::RADTODEG);
		lua_setfield(L, -2, "y");
		lua_pushnumber(L, ref->m_camera->getFovMax() * core::RADTODEG);
		lua_setfield(L, -2, "max");
	}
	lua_pushnumber(L, cam->getFOV() * core::RADTODEG);
	lua_setfield(L, -2, "actual");
	return 1;
}

// get_pos(self)
int LuaCamera::l_get_pos(lua_State *L)
{
	LuaCamera *ref = checkObject<LuaCamera>(L, 1);
	scene::ICameraSceneNode *cam = ref->m_camera ? ref->m_camera->getCameraNode() : ref->m_raw_cam;
	if (!cam)
		return 0;

	push_v3f(L, cam->getPosition() / BS);
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
	LuaCamera *ref = checkObject<LuaCamera>(L, 1);
	scene::ICameraSceneNode *cam = ref->m_camera ? ref->m_camera->getCameraNode() : ref->m_raw_cam;
	if (!cam)
		return 0;

	v3f dir = (cam->getTarget() - cam->getAbsolutePosition()).normalize();
	push_v3f(L, dir);
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
	LuaCamera *ref = checkObject<LuaCamera>(L, 1);
	scene::ICameraSceneNode *cam = ref->m_camera ? ref->m_camera->getCameraNode() : ref->m_raw_cam;
	if (!cam)
		return 0;

	lua_pushnumber(L, cam->getAspectRatio());
	return 1;
}

int LuaCamera::l_set_fov(lua_State *L)
{
	LuaCamera *ref = checkObject<LuaCamera>(L, 1);
	scene::ICameraSceneNode *cam = ref->m_camera ? ref->m_camera->getCameraNode() : ref->m_raw_cam;
	if (!cam)
		return 0;
	float fov = luaL_checknumber(L, 2);
	cam->setFOV(fov * core::DEGTORAD);
	return 0;
}

int LuaCamera::l_set_pos(lua_State *L)
{
	LuaCamera *ref = checkObject<LuaCamera>(L, 1);
	scene::ICameraSceneNode *cam = ref->m_camera ? ref->m_camera->getCameraNode() : ref->m_raw_cam;
	if (!cam)
		return 0;
	v3f pos = check_v3f(L, 2);
	cam->setPosition(pos * BS);
	return 0;
}

int LuaCamera::l_set_look_vertical(lua_State *L)
{
	LuaCamera *ref = checkObject<LuaCamera>(L, 1);
	scene::ICameraSceneNode *cam = ref->m_camera ? ref->m_camera->getCameraNode() : ref->m_raw_cam;
	if (!cam)
		return 0;
	float pitch = luaL_checknumber(L, 2);
	v3f rot = cam->getRotation();
	rot.X = pitch * core::RADTODEG; // Irrlicht uses degrees
	cam->setRotation(rot);
	return 0;
}

int LuaCamera::l_set_look_horizontal(lua_State *L)
{
	LuaCamera *ref = checkObject<LuaCamera>(L, 1);
	scene::ICameraSceneNode *cam = ref->m_camera ? ref->m_camera->getCameraNode() : ref->m_raw_cam;
	if (!cam)
		return 0;
	float yaw = luaL_checknumber(L, 2);
	v3f rot = cam->getRotation();
	rot.Y = yaw * core::RADTODEG - 90.0f;
	cam->setRotation(rot);
	return 0;
}

int LuaCamera::l_set_aspect_ratio(lua_State *L)
{
	LuaCamera *ref = checkObject<LuaCamera>(L, 1);
	scene::ICameraSceneNode *cam = ref->m_camera ? ref->m_camera->getCameraNode() : ref->m_raw_cam;
	if (!cam)
		return 0;
	float aspect = luaL_checknumber(L, 2);
	cam->setAspectRatio(aspect);
	return 0;
}

int LuaCamera::l_add(lua_State *L)
{
	scene::ISceneManager *smgr = RenderingEngine::get_scene_manager();
	scene::ICameraSceneNode *cam = smgr->addCameraSceneNode();
	if (!cam)
		return 0;

	LuaCamera *o = new LuaCamera(nullptr);
	o->m_raw_cam = cam;
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
	return 1;
}

int LuaCamera::l_remove(lua_State *L)
{
	LuaCamera *ref = checkObject<LuaCamera>(L, 1);
	if (ref->m_raw_cam) {
		ref->m_raw_cam->remove();
		ref->m_raw_cam = nullptr;
	}
	return 0;
}

int LuaCamera::l_render(lua_State *L)
{
	LuaCamera *ref = checkObject<LuaCamera>(L, 1);
	scene::ICameraSceneNode *cam = ref->m_camera ? ref->m_camera->getCameraNode() : ref->m_raw_cam;
	if (!cam)
		return 0;

	luaL_checktype(L, 2, LUA_TTABLE);
	std::string texture_name = getstringfield_default(L, 2, "texture", "");

	core::rect<s32> viewport(0, 0, 0, 0);
	bool has_viewport = false;
	lua_getfield(L, 2, "viewport");
	if (lua_istable(L, -1)) {
		viewport.UpperLeftCorner.X = getintfield_default(L, -1, "x1", 0);
		viewport.UpperLeftCorner.Y = getintfield_default(L, -1, "y1", 0);
		viewport.LowerRightCorner.X = getintfield_default(L, -1, "x2", 0);
		viewport.LowerRightCorner.Y = getintfield_default(L, -1, "y2", 0);
		has_viewport = true;
	}
	lua_pop(L, 1);

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	scene::ISceneManager *smgr = RenderingEngine::get_scene_manager();

	scene::ICameraSceneNode *old_cam = smgr->getActiveCamera();
	smgr->setActiveCamera(cam);

	core::rect<s32> old_viewport = driver->getViewPort();
	if (has_viewport) {
		driver->setViewPort(viewport);
	}

	video::ITexture *render_tex = nullptr;
	if (!texture_name.empty()) {
		render_tex = driver->getTexture(texture_name.c_str());
		if (render_tex) {
			driver->setRenderTarget(render_tex, true, true, video::SColor(0,0,0,0));
		}
	}

	smgr->drawAll();

	if (render_tex) {
		driver->setRenderTarget(0, false, false);
	}

	if (has_viewport) {
		driver->setViewPort(old_viewport);
	}

	smgr->setActiveCamera(old_cam);

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

	luamethod(LuaCamera, set_fov),
	luamethod(LuaCamera, set_pos),
	luamethod(LuaCamera, set_look_vertical),
	luamethod(LuaCamera, set_look_horizontal),
	luamethod(LuaCamera, set_aspect_ratio),
	luamethod(LuaCamera, add),
	luamethod(LuaCamera, remove),
	luamethod(LuaCamera, render),

	{0, 0}
};
