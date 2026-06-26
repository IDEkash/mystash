// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "guiUtil.h"
#include <IVideoDriver.h>
#include <vector>
#include <cmath>

namespace gui {

void drawRoundedRectangle(video::IVideoDriver *driver, const core::rect<s32> &rect,
		video::SColor color, const core::rect<s32> *clip, f32 radius)
{
	if (radius <= 0.0f) {
		driver->draw2DRectangle(color, rect, clip);
		return;
	}

	f32 r = radius;
	f32 w = rect.getWidth();
	f32 h = rect.getHeight();

	if (r * 2 > w) r = w / 2;
	if (r * 2 > h) r = h / 2;

	// Use stack-allocated vertex/index arrays for speed.
	// 8 points per corner is usually enough for smooth look.
	const int segments = 8;
	const int num_vertices = segments * 4 + 1; // + center point
	video::S3DVertex vertices[num_vertices];
	u16 indices[segments * 4 * 3];

	// Center point
	vertices[0] = video::S3DVertex(rect.getCenter().X, rect.getCenter().Y, 0, 0, 0, -1, color, 0, 0);

	struct Corner {
		f32 x, y;
		f32 start_angle;
	} corners[4] = {
		{ (f32)rect.LowerRightCorner.X - r, (f32)rect.LowerRightCorner.Y - r, 0.0f },
		{ (f32)rect.UpperLeftCorner.X + r,  (f32)rect.LowerRightCorner.Y - r, core::PI / 2.0f },
		{ (f32)rect.UpperLeftCorner.X + r,  (f32)rect.UpperLeftCorner.Y + r,  core::PI },
		{ (f32)rect.LowerRightCorner.X - r, (f32)rect.UpperLeftCorner.Y + r,  3.0f * core::PI / 2.0f }
	};

	int v_idx = 1;
	int i_idx = 0;
	for (int c = 0; c < 4; ++c) {
		for (int s = 0; s < segments; ++s) {
			f32 angle = corners[c].start_angle + (f32)s / (segments - 1) * (core::PI / 2.0f);
			f32 vx = corners[c].x + std::cos(angle) * r;
			f32 vy = corners[c].y + std::sin(angle) * r;
			vertices[v_idx] = video::S3DVertex(vx, vy, 0, 0, 0, -1, color, 0, 0);

			if (v_idx > 1) {
				indices[i_idx++] = 0;
				indices[i_idx++] = v_idx - 1;
				indices[i_idx++] = v_idx;
			}
			v_idx++;
		}
	}
	// Close the loop
	indices[i_idx++] = 0;
	indices[i_idx++] = v_idx - 1;
	indices[i_idx++] = 1;

	if (clip) {
		driver->setClipRect(*clip);
	}
	driver->draw2DVertexPrimitiveList(vertices, num_vertices, indices, segments * 4,
			video::EVT_STANDARD, scene::EPT_TRIANGLES);
	if (clip) {
		driver->setClipRect();
	}
}

} // namespace gui
