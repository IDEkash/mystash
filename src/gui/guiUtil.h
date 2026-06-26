// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <IVideoDriver.h>
#include "irr_v2d.h"
#include <vector>

namespace gui {

void drawRoundedRectangle(video::IVideoDriver *driver, const core::rect<s32> &rect,
	const video::SColor &color, float radius, const core::rect<s32> *clip = nullptr);

void drawRoundedRectangle(video::IVideoDriver *driver, const core::rect<s32> &rect,
	const video::SColor colors[4], float radius, const core::rect<s32> *clip = nullptr);

} // namespace gui
