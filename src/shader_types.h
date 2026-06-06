// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2025 Jules

#pragma once

#include <string>
#include <variant>
#include <vector>
#include "irr_v2d.h"
#include "irr_v3d.h"

typedef std::variant<float, int, bool, v2f, v3f> ModUniformValue;

struct ModShaderOverride {
	std::string name;
	std::string target;
	std::string stage; // "vertex", "fragment", or "both"
	std::string path;
	s32 priority;
};
