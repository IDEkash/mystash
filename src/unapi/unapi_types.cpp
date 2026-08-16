// Luanti Universal Engine Extension Architecture
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "unapi_types.h"

namespace unapi {

std::string permissionToString(Permission perm) {
	switch (perm) {
		case Permission::RenderAccess: return "render.access";
		case Permission::RenderCreateResources: return "render.create_resources";
		case Permission::FilesystemStorage: return "filesystem.storage";
		case Permission::NetworkHttp: return "network.http";
		case Permission::NetworkWebsocket: return "network.websocket";
		case Permission::WorldModify: return "world.modify";
		case Permission::PhysicsModify: return "physics.modify";
	}
	return "unknown";
}

Permission stringToPermission(const std::string &str) {
	if (str == "render.access") return Permission::RenderAccess;
	if (str == "render.create_resources") return Permission::RenderCreateResources;
	if (str == "filesystem.storage") return Permission::FilesystemStorage;
	if (str == "network.http") return Permission::NetworkHttp;
	if (str == "network.websocket") return Permission::NetworkWebsocket;
	if (str == "world.modify") return Permission::WorldModify;
	if (str == "physics.modify") return Permission::PhysicsModify;
	return Permission::RenderAccess;
}

std::string stabilityToString(ApiStability stab) {
	switch (stab) {
		case ApiStability::Stable: return "stable";
		case ApiStability::Experimental: return "experimental";
		case ApiStability::Internal: return "internal";
		case ApiStability::Deprecated: return "deprecated";
	}
	return "stable";
}

ApiStability stringToStability(const std::string &str) {
	if (str == "stable") return ApiStability::Stable;
	if (str == "experimental") return ApiStability::Experimental;
	if (str == "internal") return ApiStability::Internal;
	if (str == "deprecated") return ApiStability::Deprecated;
	return ApiStability::Stable;
}

std::string valueTypeToString(ValueType type) {
	switch (type) {
		case ValueType::Nil: return "nil";
		case ValueType::Boolean: return "boolean";
		case ValueType::Integer: return "integer";
		case ValueType::Float: return "float";
		case ValueType::String: return "string";
		case ValueType::Vector3: return "vector3";
		case ValueType::Vector2: return "vector2";
		case ValueType::Rotation3: return "rotation3";
		case ValueType::Transform4: return "transform4";
		case ValueType::Enum: return "enum";
		case ValueType::ResourceRef: return "resource_ref";
		case ValueType::ObjectRef: return "object_ref";
		case ValueType::Array: return "array";
		case ValueType::Map: return "map";
		case ValueType::Buffer: return "buffer";
	}
	return "unknown";
}

} // namespace unapi
