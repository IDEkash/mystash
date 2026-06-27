// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "irrTypes.h"
#include "SColor.h"
#include "rect.h"

namespace video {
	class IVideoDriver;
}

namespace gui {

void draw2DRoundedRectangle(video::IVideoDriver *driver,
		const core::rect<s32> &rect, video::SColor color,
		s32 radius, const core::rect<s32> *clip = nullptr);

} // namespace gui
