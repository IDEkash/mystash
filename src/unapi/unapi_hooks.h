// Luanti Universal Engine Extension Architecture
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "unapi_types.h"
#include <vector>
#include <functional>
#include <mutex>
#include <unordered_map>

namespace unapi {

enum class HookTiming {
	Before,
	After,
	Override
};

struct HookCallback {
	uint64_t id;
	std::string extension_id;
	HookTiming timing;
	std::function<Value(const ValueArray &args, bool &override_execution, Value &override_result)> cpp_callback;
	int lua_ref {-1}; // reference in Lua state if created from Lua
};

class HookRegistry {
private:
	std::mutex m_mutex;
	uint64_t m_next_hook_id {1};
	std::unordered_map<std::string, std::vector<HookCallback>> m_hooks;

	HookRegistry() = default;

public:
	static HookRegistry &get() {
		static HookRegistry instance;
		return instance;
	}

	uint64_t registerHook(const std::string &hook_name, const std::string &extension_id, HookTiming timing,
	                      std::function<Value(const ValueArray &, bool &, Value &)> cb, int lua_ref = -1);

	bool unregisterHook(uint64_t hook_id);

	Value executeHooks(const std::string &hook_name, ValueArray &args,
	                   std::function<Value(const ValueArray &)> default_executor = nullptr);

	bool hasHooks(const std::string &hook_name);
	std::vector<std::string> getRegisteredHooks();
};

} // namespace unapi
