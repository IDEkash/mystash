// Luanti Universal Engine Extension Architecture
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "test.h"
#include "unapi_registry.h"
#include "unapi_hooks.h"
#include "unapi_storage.h"
#include "unapi_foundations.h"
#include "filesys.h"

class TestUnapi : public TestBase {
public:
	TestUnapi() { TestManager::registerTestModule(this); }
	const char *getName() override { return "TestUnapi"; }

	void runTests(IGameDef *gamedef) override {
		unapi::UnapiRegistry::get().initializeFoundations();

		TEST(testTypeRegistrationAndIntrospection);
		TEST(testObjectCreationAndProperties);
		TEST(testHandlesAndComposition);
		TEST(testHooksSystem);
		TEST(testSandboxedStorage);
		TEST(testPermissionsSystem);
	}

private:
	void testTypeRegistrationAndIntrospection() {
		auto &reg = unapi::UnapiRegistry::get();
		UASSERT(reg.hasType("render.camera"));
		UASSERT(reg.hasType("render.target"));
		UASSERT(reg.hasType("render.pass"));
		UASSERT(reg.hasType("game.world"));

		auto types = reg.listTypes("render");
		UASSERT(!types.empty());

		unapi::ValueMap info = reg.inspectType("render.camera");
		UASSERT(info["type_name"].asString() == "render.camera");
		UASSERT(info["foundation"].asString() == "render");
	}

	void testObjectCreationAndProperties() {
		auto &reg = unapi::UnapiRegistry::get();
		auto cam = reg.createObject("render.camera", "test_ext");
		UASSERT(cam != nullptr);
		UASSERT(cam->getTypeName() == "render.camera");

		// Property read/write
		cam->setProperty("fov", unapi::Value(90.0));
		UASSERT(cam->getProperty("fov").asFloat() == 90.0);

		cam->setProperty("position", unapi::Value(unapi::Vector3f(10, 20, 30)));
		unapi::Vector3f pos = cam->getProperty("position").asVector3();
		UASSERT(pos.x == 10 && pos.y == 20 && pos.z == 30);
	}

	void testHandlesAndComposition() {
		auto &reg = unapi::UnapiRegistry::get();
		auto cam = reg.createObject("render.camera");
		uint64_t handle = cam->getHandleId();
		UASSERT(handle > 0);

		auto retrieved = reg.getObjectByHandle(handle);
		UASSERT(retrieved == cam);

		auto target = reg.createObject("render.target");
		auto pass = reg.createObject("render.pass");
		auto world = reg.createObject("game.world");

		// Cross-foundation composition
		pass->setProperty("camera", unapi::Value(cam));
		pass->setProperty("target", unapi::Value(target));
		pass->setProperty("world", unapi::Value(world));

		UASSERT(pass->getProperty("camera").asObject() == cam);
		UASSERT(pass->getProperty("target").asObject() == target);
		UASSERT(pass->getProperty("world").asObject() == world);
	}

	void testHooksSystem() {
		auto &hooks = unapi::HookRegistry::get();
		bool hook_ran = false;

		uint64_t hid = hooks.registerHook(
			"test.event", "test_ext", unapi::HookTiming::Before,
			[&hook_ran](const unapi::ValueArray &args, bool &override_exec, unapi::Value &override_res) -> unapi::Value {
				hook_ran = true;
				return unapi::Value("ok");
			}
		);

		UASSERT(hooks.hasHooks("test.event"));

		unapi::ValueArray args;
		args.push_back(unapi::Value("param1"));
		hooks.executeHooks("test.event", args);

		UASSERT(hook_ran);
		hooks.unregisterHook(hid);
	}

	void testSandboxedStorage() {
		auto &reg = unapi::UnapiRegistry::get();
		auto storage = reg.getStorage("test_extension");
		UASSERT(storage != nullptr);

		std::string test_path = "config/test.json";
		std::string content = "{\"version\": 1.0, \"setting\": true}";

		UASSERT(storage->writeFile(test_path, content));
		UASSERT(storage->exists(test_path));

		std::string read_content = storage->readFile(test_path);
		UASSERT(read_content == content);

		auto files = storage->listDirectory("config");
		UASSERT(!files.empty());

		UASSERT(storage->deleteFile(test_path));
		UASSERT(!storage->exists(test_path));
	}

	void testPermissionsSystem() {
		auto &reg = unapi::UnapiRegistry::get();
		std::string ext = "untrusted_mod";

		reg.revokePermission(ext, unapi::Permission::NetworkHttp);
		UASSERT(!reg.hasPermission(ext, unapi::Permission::NetworkHttp));

		reg.grantPermission(ext, unapi::Permission::NetworkHttp);
		UASSERT(reg.hasPermission(ext, unapi::Permission::NetworkHttp));
	}
};

static TestUnapi g_test_unapi_instance;
