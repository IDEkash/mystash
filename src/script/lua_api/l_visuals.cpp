#include "l_visuals.h"

#define API_FCT(name) registerFunction(L, #name, l_##name, visuals)

#include "l_internal.h"
#include "client/client.h"
#include "client/visuals_manager.h"
#include "common/c_converter.h"

int ModApiVisuals::l_add_post_processing_pass(lua_State *L)
{
	Client *client = getClient(L);
	if (!client->getVisualsManager())
		return 0;

	std::string shader_name = luaL_checkstring(L, 1);
	std::vector<u8> texture_map;
	if (lua_istable(L, 2)) {
		lua_pushnil(L);
		while (lua_next(L, 2) != 0) {
			texture_map.push_back((u8)lua_tointeger(L, -1));
			lua_pop(L, 1);
		}
	} else {
		// Default to texture 0 (rendered scene)
		texture_map.push_back(0);
	}

	LuaShaderUniformSetter *setter = new LuaShaderUniformSetter();
	u32 shader_id = client->getShaderSource()->getShader(shader_name, ShaderConstants(), video::EMT_SOLID, setter);

	u32 id = client->getVisualsManager()->addPass(shader_name, texture_map, shader_id, setter);
	lua_pushinteger(L, id);
	return 1;
}

int ModApiVisuals::l_remove_post_processing_pass(lua_State *L)
{
	Client *client = getClient(L);
	if (!client->getVisualsManager())
		return 0;

	u32 id = (u32)luaL_checkinteger(L, 1);
	client->getVisualsManager()->removePass(id);
	return 0;
}

int ModApiVisuals::l_set_post_processing_uniform(lua_State *L)
{
	Client *client = getClient(L);
	if (!client->getVisualsManager())
		return 0;

	u32 id = (u32)luaL_checkinteger(L, 1);
	std::string name = luaL_checkstring(L, 2);

	CustomPass *pass = client->getVisualsManager()->getPass(id);
	if (!pass)
		return 0;

	if (lua_isnumber(L, 3)) {
		pass->setter->setUniform(name, (float)lua_tonumber(L, 3));
	} else if (lua_istable(L, 3)) {
		video::SColor color;
		if (read_color(L, 3, &color)) {
			pass->setter->setUniform(name, video::SColorf(color));
		} else {
			lua_rawgeti(L, 3, 1);
			int n = 0;
			if (lua_isnumber(L, -1)) {
				// Count elements
				n = lua_objlen(L, 3);
			}
			lua_pop(L, 1);

			if (n == 2) pass->setter->setUniform(name, read_v2f(L, 3));
			else if (n == 3) pass->setter->setUniform(name, read_v3f(L, 3));
			else if (n == 16) {
				core::matrix4 m;
				for (int i=0; i<16; i++) {
					lua_rawgeti(L, 3, i+1);
					m[i] = (float)lua_tonumber(L, -1);
					lua_pop(L, 1);
				}
				pass->setter->setUniform(name, m);
			}
		}
	}

	return 0;
}

void ModApiVisuals::Initialize(lua_State *L, int top)
{
	lua_newtable(L);
	int visuals = lua_gettop(L);

	API_FCT(add_post_processing_pass);
	API_FCT(remove_post_processing_pass);
	API_FCT(set_post_processing_uniform);

	lua_setfield(L, top, "visuals");
}
