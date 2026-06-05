// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2024 Jules

#pragma once

#include <string>
#include <vector>
#include <map>
#include <variant>
#include "irr_v2d.h"
#include "irr_v3d.h"

typedef std::variant<float, int, v2f, v3f> UniformValue;
typedef std::map<std::string, UniformValue> ShaderUniforms;

struct ModShaderOverride {
	std::string name; // mod shader name (e.g. "cool_shaders.nodes")
	std::string target; // engine shader target (e.g. "nodes_shader")
	std::string vertex_path;
	std::string fragment_path;
	int priority;
};

extern const std::vector<std::string> overridable_shaders;
