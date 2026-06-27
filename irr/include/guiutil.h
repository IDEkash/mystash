// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "irrlichttypes_bloated.h"
#include <IVideoDriver.h>

namespace gui {

void draw2DRoundedRectangle(video::IVideoDriver *driver,
		const core::rect<s32> &rect, video::SColor color,
		s32 radius, const core::rect<s32> *clip = nullptr);

} // namespace gui
