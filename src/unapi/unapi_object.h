// Luanti Universal Engine Extension Architecture
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "unapi_types.h"
#include <mutex>
#include <atomic>

namespace unapi {

class UnapiObject : public std::enable_shared_from_this<UnapiObject> {
protected:
	std::string m_type_name;
	uint64_t m_handle_id {0};
	std::unordered_map<std::string, Value> m_properties;
	std::unordered_map<std::string, ObjectRefPtr> m_children;
	std::mutex m_mutex;

	using PropertyGetter = std::function<Value(const UnapiObject*)>;
	using PropertySetter = std::function<bool(UnapiObject*, const Value&)>;
	using MethodHandler = std::function<Value(UnapiObject*, const ValueArray&)>;

	std::unordered_map<std::string, PropertyGetter> m_property_getters;
	std::unordered_map<std::string, PropertySetter> m_property_setters;
	std::unordered_map<std::string, MethodHandler> m_methods;

public:
	UnapiObject(const std::string &type_name);
	virtual ~UnapiObject() = default;

	const std::string &getTypeName() const { return m_type_name; }
	uint64_t getHandleId() const { return m_handle_id; }
	void setHandleId(uint64_t id) { m_handle_id = id; }

	// Generic property access
	virtual Value getProperty(const std::string &prop_name);
	virtual bool setProperty(const std::string &prop_name, const Value &val);

	// Generic function invocation
	virtual Value invokeMethod(const std::string &method_name, const ValueArray &args);

	// Child management
	void addChild(const std::string &key, ObjectRefPtr child);
	ObjectRefPtr getChild(const std::string &key);
	void removeChild(const std::string &key);
	std::vector<std::string> getChildKeys();

	// Registration helpers for derived C++ object classes
	void registerGetter(const std::string &prop_name, PropertyGetter getter);
	void registerSetter(const std::string &prop_name, PropertySetter setter);
	void registerMethod(const std::string &method_name, MethodHandler handler);

	// Fast handle lookup support
	virtual void resetState() {}
};

} // namespace unapi
