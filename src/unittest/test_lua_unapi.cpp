// Luanti Universal Engine Extension Architecture
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "test.h"
#include "unapi_registry.h"
#include "lua_api/l_unapi.h"
#include "log.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

class TestLuaUnapi : public TestBase {
public:
	TestLuaUnapi() { TestManager::registerTestModule(this); }
	const char *getName() override { return "TestLuaUnapi"; }

	void runTests(IGameDef *gamedef) override {
		TEST(testLuaUnapiCoreAndValidations);
	}

private:
	void testLuaUnapiCoreAndValidations() {
		lua_State *L = luaL_newstate();
		luaL_openlibs(L);

		lua_newtable(L);
		lua_setglobal(L, "core");

		unapi::UnapiRegistry::get().initializeFoundations();
		ModApiUnapi::Initialize(L, 0);

		const char *script = R"lua(
			-- Test 1: Introspection and reflection
			local cam_info = unapi.inspect("render.camera")
			assert(cam_info ~= nil, "render.camera info should exist")
			assert(cam_info.type_name == "render.camera", "type name mismatch")
			assert(cam_info.foundation == "render", "foundation mismatch")
			assert(cam_info.properties.position ~= nil, "position prop missing")
			assert(cam_info.properties.fov ~= nil, "fov prop missing")

			-- Test 2: Object Creation & Property Manipulation
			local camera = unapi.create("render.camera")
			assert(camera ~= nil, "failed to create render.camera")
			camera.fov = 85.0
			assert(camera.fov == 85.0, "fov property set/get failed")

			camera.position = {x = 10, y = 20, z = 30}
			local pos = camera.position
			assert(pos.x == 10 and pos.y == 20 and pos.z == 30, "vector3 position failed")

			-- Test 3: Multi-Camera Validation Addon
			local sec_camera = unapi.create("render.camera")
			sec_camera.position = {x = 100, y = 50, z = -200}
			sec_camera.rotation = {x = 0, y = 45, z = 0}
			sec_camera.fov = 90.0

			local sec_target = unapi.create("render.target")
			sec_target:resize(1280, 720)
			assert(sec_target.width == 1280 and sec_target.height == 720, "render target resize failed")

			local sec_pass = unapi.create("render.pass")
			sec_pass.camera = sec_camera
			sec_pass.target = sec_target

			local pass_executed = false
			local hook_id = unapi.hook("render.pass.execute", function(pass_id, cam, tgt)
				pass_executed = true
				return true, true
			end)

			sec_pass:execute()
			assert(pass_executed, "multi-camera render pass execution failed")
			unapi.unhook(hook_id)

			-- Test 4: Render-To-Texture & Custom Rendering Composition
			local rtt_camera = unapi.create("render.camera")
			local rtt_target = unapi.create("render.target")
			local rtt_pass = unapi.create("render.pass")
			local rtt_material = unapi.create("asset.material")

			rtt_pass.camera = rtt_camera
			rtt_pass.target = rtt_target
			local tex = rtt_target.texture
			assert(tex ~= nil, "render target texture missing")

			rtt_material.texture = tex
			assert(rtt_material.texture == tex, "composition material texture binding failed")

			-- Test 5: Cross-World Rendering Validation
			local main_world = unapi.create("game.world")
			main_world.name = "overworld"
			main_world.instance_id = 0

			local dest_world = unapi.create("game.world")
			dest_world.name = "nether_dimension"
			dest_world.instance_id = 1

			local dim_camera = unapi.create("render.camera")
			dim_camera.world = dest_world
			assert(dim_camera.world == dest_world, "cross-world camera binding failed")

			-- Test 6: Sandboxed Persistent Storage
			local storage = unapi.storage.get("validation_addon")
			assert(storage ~= nil, "storage object missing")
			local test_file = "cache/state.json"
			assert(storage:write(test_file, '{"saved": true}'), "storage write failed")
			assert(storage:exists(test_file), "storage exists failed")
			local content = storage:read(test_file)
			assert(content == '{"saved": true}', "storage read content mismatch")
			assert(storage:delete(test_file), "storage delete failed")

			-- Test 7: Immersive Portals Proof-Of-Concept Extension
			local PortalExtension = {
				destination_world = dest_world,
				entrance_pos = {x = 0, y = 1, z = 0},
				exit_pos = {x = 1000, y = 500, z = 1000},
				exit_yaw_offset = 180.0,
				camera = unapi.create("render.camera"),
				target = unapi.create("render.target"),
				pass = unapi.create("render.pass"),
				material = unapi.create("asset.material"),
				max_recursion = 3
			}

			function PortalExtension:init()
				self.camera.world = self.destination_world
				self.pass.camera = self.camera
				self.pass.target = self.target
				self.pass.world = self.destination_world
				self.pass.max_recursion_depth = self.max_recursion
				self.material.texture = self.target.texture
			end

			function PortalExtension:update_portal_camera(player_cam_pos, player_cam_rot)
				-- Calculate portal destination relative transform
				local dx = player_cam_pos.x - self.entrance_pos.x
				local dy = player_cam_pos.y - self.entrance_pos.y
				local dz = player_cam_pos.z - self.entrance_pos.z

				-- Apply exit transform and rotation offset
				local dest_x = self.exit_pos.x + dx
				local dest_y = self.exit_pos.y + dy
				local dest_z = self.exit_pos.z + dz
				local dest_rot_y = player_cam_rot.y + self.exit_yaw_offset

				self.camera.position = {x = dest_x, y = dest_y, z = dest_z}
				self.camera.rotation = {x = player_cam_rot.x, y = dest_rot_y, z = player_cam_rot.z}
			end

			function PortalExtension:render_portal_surface()
				return self.pass:execute()
			end

			PortalExtension:init()
			PortalExtension:update_portal_camera({x = 5, y = 2, z = 10}, {x = 10, y = 45, z = 0})

			assert(PortalExtension.camera.position.x == 1005, "portal destination x failed")
			assert(PortalExtension.camera.position.y == 501, "portal destination y failed")
			assert(PortalExtension.camera.position.z == 1010, "portal destination z failed")

			local portal_rendered = false
			local portal_hook = unapi.hook("render.pass.execute", function(pass_id, cam, tgt)
				portal_rendered = true
				return true, true
			end)

			local ok = PortalExtension:render_portal_surface()
			assert(ok and portal_rendered, "portal pass execution failed")
			unapi.unhook(portal_hook)

			-- Verify recursion limit protection
			PortalExtension.pass.recursion_depth = 3
			local ok_limit = PortalExtension:render_portal_surface()
			assert(ok_limit == false, "recursion depth limit failed to block infinite loop")

			print("[LUA UNAPI TEST] All Universal Extension API Lua validations passed successfully!")
		)lua";

		lua_getglobal(L, "debug");
		lua_getfield(L, -1, "traceback");
		lua_remove(L, -2); // remove debug

		int errfunc = lua_gettop(L);

		int status = luaL_loadstring(L, script);
		if (status == 0) {
			status = lua_pcall(L, 0, 0, errfunc);
		}

		if (status != 0) {
			std::string err = lua_tostring(L, -1);
			errorstream << "Lua UNAPI Test Failed:\n" << err << std::endl;
			lua_close(L);
			UASSERT(false);
			return;
		}

		lua_close(L);
	}
};

static TestLuaUnapi g_test_lua_unapi_instance;
