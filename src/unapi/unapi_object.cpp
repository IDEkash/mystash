// Luanti Universal Engine Extension Architecture
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "unapi_object.h"

namespace unapi {

UnapiObject::UnapiObject(const std::string &type_name)
	: m_type_name(type_name) {
}

Value UnapiObject::getProperty(const std::string &prop_name) {
	std::lock_guard<std::mutex> lock(m_mutex);
	auto git = m_property_getters.find(prop_name);
	if (git != m_property_getters.end()) {
		return git->second(this);
	}
	auto it = m_properties.find(prop_name);
	if (it != m_properties.end()) {
		return it->second;
	}
	return Value();
}

bool UnapiObject::setProperty(const std::string &prop_name, const Value &val) {
	std::lock_guard<std::mutex> lock(m_mutex);
	auto sit = m_property_setters.find(prop_name);
	if (sit != m_property_setters.end()) {
		return sit->second(this, val);
	}
	m_properties[prop_name] = val;
	return true;
}

Value UnapiObject::invokeMethod(const std::string &method_name, const ValueArray &args) {
	MethodHandler handler;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_methods.find(method_name);
		if (it != m_methods.end()) {
			handler = it->second;
		}
	}
	if (handler) {
		return handler(this, args);
	}
	return Value();
}

void UnapiObject::addChild(const std::string &key, ObjectRefPtr child) {
	std::lock_guard<std::mutex> lock(m_mutex);
	m_children[key] = child;
}

ObjectRefPtr UnapiObject::getChild(const std::string &key) {
	std::lock_guard<std::mutex> lock(m_mutex);
	auto it = m_children.find(key);
	if (it != m_children.end()) {
		return it->second;
	}
	return nullptr;
}

void UnapiObject::removeChild(const std::string &key) {
	std::lock_guard<std::mutex> lock(m_mutex);
	m_children.erase(key);
}

std::vector<std::string> UnapiObject::getChildKeys() {
	std::lock_guard<std::mutex> lock(m_mutex);
	std::vector<std::string> keys;
	keys.reserve(m_children.size());
	for (const auto &pair : m_children) {
		keys.push_back(pair.first);
	}
	return keys;
}

void UnapiObject::registerGetter(const std::string &prop_name, PropertyGetter getter) {
	std::lock_guard<std::mutex> lock(m_mutex);
	m_property_getters[prop_name] = getter;
}

void UnapiObject::registerSetter(const std::string &prop_name, PropertySetter setter) {
	std::lock_guard<std::mutex> lock(m_mutex);
	m_property_setters[prop_name] = setter;
}

void UnapiObject::registerMethod(const std::string &method_name, MethodHandler handler) {
	std::lock_guard<std::mutex> lock(m_mutex);
	m_methods[method_name] = handler;
}

} // namespace unapi
