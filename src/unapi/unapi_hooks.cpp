// Luanti Universal Engine Extension Architecture
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "unapi_hooks.h"
#include "log.h"

namespace unapi {

uint64_t HookRegistry::registerHook(const std::string &hook_name, const std::string &extension_id, HookTiming timing,
                                     std::function<Value(const ValueArray &, bool &, Value &)> cb, int lua_ref) {
	std::lock_guard<std::mutex> lock(m_mutex);
	uint64_t id = m_next_hook_id++;
	HookCallback callback { id, extension_id, timing, cb, lua_ref };
	m_hooks[hook_name].push_back(callback);
	infostream << "[UNAPI] Registered hook " << hook_name << " (ID: " << id << ") for extension: " << extension_id << std::endl;
	return id;
}

bool HookRegistry::unregisterHook(uint64_t hook_id) {
	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto &pair : m_hooks) {
		auto &list = pair.second;
		for (auto it = list.begin(); it != list.end(); ++it) {
			if (it->id == hook_id) {
				list.erase(it);
				return true;
			}
		}
	}
	return false;
}

bool HookRegistry::hasHooks(const std::string &hook_name) {
	std::lock_guard<std::mutex> lock(m_mutex);
	auto it = m_hooks.find(hook_name);
	return (it != m_hooks.end() && !it->second.empty());
}

std::vector<std::string> HookRegistry::getRegisteredHooks() {
	std::lock_guard<std::mutex> lock(m_mutex);
	std::vector<std::string> names;
	for (const auto &pair : m_hooks) {
		if (!pair.second.empty())
			names.push_back(pair.first);
	}
	return names;
}

Value HookRegistry::executeHooks(const std::string &hook_name, ValueArray &args,
                                  std::function<Value(const ValueArray &)> default_executor) {
	std::vector<HookCallback> current_hooks;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_hooks.find(hook_name);
		if (it != m_hooks.end()) {
			current_hooks = it->second;
		}
	}

	if (current_hooks.empty()) {
		if (default_executor) {
			return default_executor(args);
		}
		return Value();
	}

	bool overridden = false;
	Value override_result;

	// 1. Run 'Before' timing hooks
	for (const auto &hook : current_hooks) {
		if (hook.timing == HookTiming::Before && hook.cpp_callback) {
			bool req_override = false;
			Value res = hook.cpp_callback(args, req_override, override_result);
			if (req_override) {
				overridden = true;
			}
		}
	}

	// 2. Run 'Override' timing hooks if any exist and not already overridden
	for (const auto &hook : current_hooks) {
		if (hook.timing == HookTiming::Override && hook.cpp_callback && !overridden) {
			bool req_override = false;
			Value res = hook.cpp_callback(args, req_override, override_result);
			if (req_override) {
				overridden = true;
				override_result = res;
			}
		}
	}

	// 3. Run default executor if not overridden
	Value result;
	if (overridden) {
		result = override_result;
	} else if (default_executor) {
		result = default_executor(args);
	}

	// 4. Run 'After' timing hooks
	for (const auto &hook : current_hooks) {
		if (hook.timing == HookTiming::After && hook.cpp_callback) {
			bool dummy_override = false;
			Value dummy_result;
			hook.cpp_callback(args, dummy_override, dummy_result);
		}
	}

	return result;
}

} // namespace unapi
