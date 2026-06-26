// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <irrlicht.h>
#include "irrlichttypes_bloated.h"

namespace gui {

void drawRoundedRectangle(video::IVideoDriver *driver, const core::rect<s32> &rect,
		video::SColor color, const core::rect<s32> *clip = nullptr, f32 radius = 0.0f);

} // namespace gui
