// Luanti Universal Engine Extension Architecture
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <memory>
#include <functional>
#include "irr_v3d.h"
#include "irr_v2d.h"
#include "irr_aabb3d.h"

namespace unapi {

enum class ValueType {
	Nil,
	Boolean,
	Integer,
	Float,
	String,
	Vector3,
	Vector2,
	Rotation3,
	Transform4,
	Enum,
	ResourceRef,
	ObjectRef,
	Array,
	Map,
	Buffer
};

enum class PropertyAccess {
	ReadOnly,
	WriteOnly,
	ReadWrite
};

enum class ApiStability {
	Stable,
	Experimental,
	Internal,
	Deprecated
};

enum class Permission {
	RenderAccess,
	RenderCreateResources,
	FilesystemStorage,
	NetworkHttp,
	NetworkWebsocket,
	WorldModify,
	PhysicsModify
};

struct Vector3f {
	float x {0.0f}, y {0.0f}, z {0.0f};
	Vector3f() = default;
	Vector3f(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
	Vector3f(v3f v) : x(v.X), y(v.Y), z(v.Z) {}
	v3f toV3f() const { return v3f(x, y, z); }
};

struct Vector2f {
	float x {0.0f}, y {0.0f};
	Vector2f() = default;
	Vector2f(float _x, float _y) : x(_x), y(_y) {}
	Vector2f(v2f v) : x(v.X), y(v.Y) {}
	v2f toV2f() const { return v2f(x, y); }
};

struct TransformMatrix {
	float m[16] {
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1
	};
};

class UnapiObject;

using ObjectRefPtr = std::shared_ptr<UnapiObject>;

struct Value;

using ValueArray = std::vector<Value>;
using ValueMap = std::unordered_map<std::string, Value>;

struct Value {
	ValueType type {ValueType::Nil};
	std::variant<
		bool,
		int64_t,
		double,
		std::string,
		Vector3f,
		Vector2f,
		TransformMatrix,
		ObjectRefPtr,
		std::shared_ptr<ValueArray>,
		std::shared_ptr<ValueMap>,
		std::vector<uint8_t>
	> data;

	Value() = default;
	Value(bool b) : type(ValueType::Boolean), data(b) {}
	Value(int64_t i) : type(ValueType::Integer), data(i) {}
	Value(uint64_t u) : type(ValueType::Integer), data(static_cast<int64_t>(u)) {}
	Value(int i) : type(ValueType::Integer), data(static_cast<int64_t>(i)) {}
	Value(uint32_t u) : type(ValueType::Integer), data(static_cast<int64_t>(u)) {}
	Value(double d) : type(ValueType::Float), data(d) {}
	Value(float f) : type(ValueType::Float), data(static_cast<double>(f)) {}
	Value(const std::string &s) : type(ValueType::String), data(s) {}
	Value(const char *s) : type(ValueType::String), data(std::string(s)) {}
	Value(Vector3f v) : type(ValueType::Vector3), data(v) {}
	Value(Vector2f v) : type(ValueType::Vector2), data(v) {}
	Value(TransformMatrix t) : type(ValueType::Transform4), data(t) {}
	Value(ObjectRefPtr obj) : type(ValueType::ObjectRef), data(obj) {}
	Value(ValueArray arr) : type(ValueType::Array), data(std::make_shared<ValueArray>(arr)) {}
	Value(ValueMap map) : type(ValueType::Map), data(std::make_shared<ValueMap>(map)) {}
	Value(std::vector<uint8_t> buf) : type(ValueType::Buffer), data(buf) {}

	bool isNil() const { return type == ValueType::Nil; }
	bool asBool(bool def = false) const {
		if (type == ValueType::Boolean) return std::get<bool>(data);
		return def;
	}
	int64_t asInt(int64_t def = 0) const {
		if (type == ValueType::Integer) return std::get<int64_t>(data);
		if (type == ValueType::Float) return static_cast<int64_t>(std::get<double>(data));
		return def;
	}
	double asFloat(double def = 0.0) const {
		if (type == ValueType::Float) return std::get<double>(data);
		if (type == ValueType::Integer) return static_cast<double>(std::get<int64_t>(data));
		return def;
	}
	std::string asString(const std::string &def = "") const {
		if (type == ValueType::String) return std::get<std::string>(data);
		return def;
	}
	Vector3f asVector3(Vector3f def = {}) const {
		if (type == ValueType::Vector3) return std::get<Vector3f>(data);
		return def;
	}
	Vector2f asVector2(Vector2f def = {}) const {
		if (type == ValueType::Vector2) return std::get<Vector2f>(data);
		return def;
	}
	ObjectRefPtr asObject() const {
		if (type == ValueType::ObjectRef) return std::get<ObjectRefPtr>(data);
		return nullptr;
	}
};

struct PropertyInfo {
	std::string name;
	ValueType type {ValueType::Nil};
	PropertyAccess access {PropertyAccess::ReadWrite};
	ApiStability stability {ApiStability::Stable};
	Permission required_permission {Permission::RenderAccess};
	std::string description;
};

struct FunctionParamInfo {
	std::string name;
	ValueType type {ValueType::Nil};
	bool optional {false};
	std::string description;
};

struct FunctionInfo {
	std::string name;
	std::vector<FunctionParamInfo> params;
	ValueType return_type {ValueType::Nil};
	Permission required_permission {Permission::RenderAccess};
	ApiStability stability {ApiStability::Stable};
	bool thread_safe {false};
	std::string description;
};

struct EventInfo {
	std::string name;
	std::vector<FunctionParamInfo> params;
	ApiStability stability {ApiStability::Stable};
	std::string description;
};

struct TypeDescriptor {
	std::string type_name; // e.g. "render.camera", "game.world"
	std::string foundation; // "render", "game", "asset"
	ApiStability stability {ApiStability::Stable};
	std::string description;
	std::unordered_map<std::string, PropertyInfo> properties;
	std::unordered_map<std::string, FunctionInfo> functions;
	std::unordered_map<std::string, EventInfo> events;
	std::vector<Permission> required_permissions;
	bool instantiable {true};
};

std::string permissionToString(Permission perm);
Permission stringToPermission(const std::string &str);
std::string stabilityToString(ApiStability stab);
ApiStability stringToStability(const std::string &str);
std::string valueTypeToString(ValueType type);

} // namespace unapi
