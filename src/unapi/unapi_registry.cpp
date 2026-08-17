// Luanti Universal Engine Extension Architecture
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "unapi_registry.h"
#include "unapi_foundations.h"
#include "log.h"

namespace unapi {

void UnapiRegistry::registerType(const TypeDescriptor &desc, std::function<ObjectRefPtr()> factory) {
	std::lock_guard<std::mutex> lock(m_mutex);
	m_types[desc.type_name] = desc;
	if (factory) {
		m_factories[desc.type_name] = factory;
	}
	infostream << "[UNAPI] Registered extension type: " << desc.type_name
	           << " (" << desc.foundation << ")" << std::endl;
}

bool UnapiRegistry::hasType(const std::string &type_name) const {
	// Const method, lock needed if m_types changed dynamically
	auto &self = const_cast<UnapiRegistry&>(*this);
	std::lock_guard<std::mutex> lock(self.m_mutex);
	return self.m_types.find(type_name) != self.m_types.end();
}

TypeDescriptor UnapiRegistry::getTypeDescriptor(const std::string &type_name) const {
	auto &self = const_cast<UnapiRegistry&>(*this);
	std::lock_guard<std::mutex> lock(self.m_mutex);
	auto it = self.m_types.find(type_name);
	if (it != self.m_types.end()) {
		return it->second;
	}
	return TypeDescriptor();
}

std::vector<std::string> UnapiRegistry::listTypes(const std::string &foundation_filter) const {
	auto &self = const_cast<UnapiRegistry&>(*this);
	std::lock_guard<std::mutex> lock(self.m_mutex);
	std::vector<std::string> result;
	for (const auto &pair : self.m_types) {
		if (foundation_filter.empty() || pair.second.foundation == foundation_filter) {
			result.push_back(pair.first);
		}
	}
	return result;
}

ObjectRefPtr UnapiRegistry::createObject(const std::string &type_name, const std::string &extension_id) {
	std::function<ObjectRefPtr()> factory;
	TypeDescriptor desc;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_factories.find(type_name);
		if (it != m_factories.end()) {
			factory = it->second;
		}
		auto tit = m_types.find(type_name);
		if (tit != m_types.end()) {
			desc = tit->second;
		}
	}

	if (!factory) {
		// Fallback generic UnapiObject instance
		factory = [type_name]() {
			return std::make_shared<UnapiObject>(type_name);
		};
	}

	ObjectRefPtr obj = factory();
	if (obj) {
		registerObjectInstance(obj);
	}
	return obj;
}

void UnapiRegistry::registerObjectInstance(ObjectRefPtr obj) {
	if (!obj) return;
	uint64_t handle = m_next_handle_id++;
	obj->setHandleId(handle);
	std::lock_guard<std::mutex> lock(m_mutex);
	m_objects_by_handle[handle] = obj;
}

ObjectRefPtr UnapiRegistry::getObjectByHandle(uint64_t handle_id) {
	std::lock_guard<std::mutex> lock(m_mutex);
	auto it = m_objects_by_handle.find(handle_id);
	if (it != m_objects_by_handle.end()) {
		return it->second;
	}
	return nullptr;
}

bool UnapiRegistry::destroyObject(uint64_t handle_id) {
	std::lock_guard<std::mutex> lock(m_mutex);
	auto it = m_objects_by_handle.find(handle_id);
	if (it != m_objects_by_handle.end()) {
		it->second->resetState();
		m_objects_by_handle.erase(it);
		return true;
	}
	return false;
}

void UnapiRegistry::grantPermission(const std::string &extension_id, Permission perm) {
	std::lock_guard<std::mutex> lock(m_mutex);
	m_extension_permissions[extension_id].insert(perm);
}

void UnapiRegistry::revokePermission(const std::string &extension_id, Permission perm) {
	std::lock_guard<std::mutex> lock(m_mutex);
	m_extension_permissions[extension_id].erase(perm);
}

bool UnapiRegistry::hasPermission(const std::string &extension_id, Permission perm) const {
	if (extension_id.empty() || extension_id == "builtin" || extension_id == "core") {
		return true; // Trusted core
	}
	auto &self = const_cast<UnapiRegistry&>(*this);
	std::lock_guard<std::mutex> lock(self.m_mutex);
	auto it = self.m_extension_permissions.find(extension_id);
	if (it != self.m_extension_permissions.end()) {
		return it->second.find(perm) != it->second.end();
	}
	return true; // Default permissive for development or as configured
}

std::shared_ptr<ExtensionStorage> UnapiRegistry::getStorage(const std::string &extension_id) {
	std::lock_guard<std::mutex> lock(m_mutex);
	auto it = m_extension_storages.find(extension_id);
	if (it != m_extension_storages.end()) {
		return it->second;
	}
	auto storage = std::make_shared<ExtensionStorage>(extension_id, "");
	m_extension_storages[extension_id] = storage;
	return storage;
}

ValueMap UnapiRegistry::inspectType(const std::string &type_name) const {
	TypeDescriptor desc = getTypeDescriptor(type_name);
	ValueMap result;
	result["type_name"] = Value(desc.type_name);
	result["foundation"] = Value(desc.foundation);
	result["stability"] = Value(stabilityToString(desc.stability));
	result["description"] = Value(desc.description);
	result["instantiable"] = Value(desc.instantiable);

	ValueMap props;
	for (const auto &pair : desc.properties) {
		ValueMap pinfo;
		pinfo["type"] = Value(valueTypeToString(pair.second.type));
		pinfo["stability"] = Value(stabilityToString(pair.second.stability));
		pinfo["permission"] = Value(permissionToString(pair.second.required_permission));
		pinfo["description"] = Value(pair.second.description);
		props[pair.first] = Value(pinfo);
	}
	result["properties"] = Value(props);

	ValueMap funcs;
	for (const auto &pair : desc.functions) {
		ValueMap finfo;
		finfo["return_type"] = Value(valueTypeToString(pair.second.return_type));
		finfo["stability"] = Value(stabilityToString(pair.second.stability));
		finfo["permission"] = Value(permissionToString(pair.second.required_permission));
		finfo["description"] = Value(pair.second.description);
		finfo["thread_safe"] = Value(pair.second.thread_safe);
		funcs[pair.first] = Value(finfo);
	}
	result["functions"] = Value(funcs);

	ValueMap evts;
	for (const auto &pair : desc.events) {
		ValueMap einfo;
		einfo["stability"] = Value(stabilityToString(pair.second.stability));
		einfo["description"] = Value(pair.second.description);
		evts[pair.first] = Value(einfo);
	}
	result["events"] = Value(evts);

	return result;
}

ValueMap UnapiRegistry::inspectObject(ObjectRefPtr obj) const {
	if (!obj) return ValueMap();
	ValueMap result = inspectType(obj->getTypeName());
	result["handle_id"] = Value(static_cast<int64_t>(obj->getHandleId()));

	ValueArray child_keys;
	for (const auto &key : obj->getChildKeys()) {
		child_keys.push_back(Value(key));
	}
	result["child_keys"] = Value(child_keys);
	return result;
}

void UnapiRegistry::initializeFoundations() {
	static bool initialized = false;
	if (initialized) return;
	initialized = true;
	registerAllFoundationTypes();
}

} // namespace unapi
