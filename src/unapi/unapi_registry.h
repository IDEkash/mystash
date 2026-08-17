// Luanti Universal Engine Extension Architecture
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "unapi_types.h"
#include "unapi_object.h"
#include "unapi_storage.h"
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <memory>
#include <atomic>

namespace unapi {

class UnapiRegistry {
private:
	std::mutex m_mutex;
	std::unordered_map<std::string, TypeDescriptor> m_types;
	std::unordered_map<std::string, std::function<ObjectRefPtr()>> m_factories;
	std::unordered_map<uint64_t, ObjectRefPtr> m_objects_by_handle;
	std::unordered_map<std::string, std::unordered_set<Permission>> m_extension_permissions;
	std::unordered_map<std::string, std::shared_ptr<ExtensionStorage>> m_extension_storages;
	std::atomic<uint64_t> m_next_handle_id {1000};

	UnapiRegistry() = default;

public:
	static UnapiRegistry &get() {
		static UnapiRegistry instance;
		return instance;
	}

	// Type registration
	void registerType(const TypeDescriptor &desc, std::function<ObjectRefPtr()> factory = nullptr);
	bool hasType(const std::string &type_name) const;
	TypeDescriptor getTypeDescriptor(const std::string &type_name) const;
	std::vector<std::string> listTypes(const std::string &foundation_filter = "") const;

	// Object creation & Handle management
	ObjectRefPtr createObject(const std::string &type_name, const std::string &extension_id = "");
	void registerObjectInstance(ObjectRefPtr obj);
	ObjectRefPtr getObjectByHandle(uint64_t handle_id);
	bool destroyObject(uint64_t handle_id);

	// Permissions
	void grantPermission(const std::string &extension_id, Permission perm);
	void revokePermission(const std::string &extension_id, Permission perm);
	bool hasPermission(const std::string &extension_id, Permission perm) const;

	// Storage
	std::shared_ptr<ExtensionStorage> getStorage(const std::string &extension_id);

	// Introspection / Reflection helper
	ValueMap inspectType(const std::string &type_name) const;
	ValueMap inspectObject(ObjectRefPtr obj) const;

	// Registration of foundational types
	void initializeFoundations();
};

} // namespace unapi
