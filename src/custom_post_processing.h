// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2024 Jules

#pragma once

#include "irrlichttypes_bloated.h"
#include "video/SColor.h"
#include <string>
#include <vector>
#include <map>
#include <variant>

using CustomUniformValue = std::variant<float, v2f, v3f, video::SColorf>;

struct CustomPostProcessingStage {
	std::string name;
	std::string shader_name;
	std::vector<u8> texture_map;
	std::map<std::string, CustomUniformValue> uniforms;
};
