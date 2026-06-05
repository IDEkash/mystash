// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2024 Jules

#include "shader_shared.h"

const std::vector<std::string> overridable_shaders = {
	"nodes_shader", "object_shader", "cloud_shader", "shadow", "second_stage",
	"bloom_downsample", "bloom_upsample", "blur_h", "blur_v", "fxaa",
	"stars_shader", "minimap_shader", "extract_bloom", "update_exposure"
};
