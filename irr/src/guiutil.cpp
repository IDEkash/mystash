// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "guiutil.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include "irrMath.h"

namespace gui {

void draw2DRoundedRectangle(video::IVideoDriver *driver,
		const core::rect<s32> &rect, video::SColor color,
		s32 radius, const core::rect<s32> *clip)
{
	if (!driver)
		return;

	if (radius <= 0) {
		driver->draw2DRectangle(color, rect, clip);
		return;
	}

	radius = std::min({radius, rect.getWidth() / 2, rect.getHeight() / 2});

	std::vector<video::S3DVertex> vertices;
	std::vector<u16> indices;

	auto add_arc = [&](core::vector2di center, float start_angle, float end_angle) {
		const int segments = 8;
		for (int i = 0; i <= segments; ++i) {
			float angle = start_angle + (end_angle - start_angle) * i / segments;
			float x = center.X + radius * std::cos(angle);
			float y = center.Y + radius * std::sin(angle);
			vertices.emplace_back(x, y, 0, 0, 0, 0, color, 0, 0);
		}
	};

	// Center point for the triangle fan
	vertices.emplace_back(rect.getCenter().X, rect.getCenter().Y, 0, 0, 0, 0, color, 0, 0);

	// Top-left
	add_arc({rect.UpperLeftCorner.X + radius, rect.UpperLeftCorner.Y + radius}, core::PI, 1.5f * core::PI);
	// Top-right
	add_arc({rect.LowerRightCorner.X - radius, rect.UpperLeftCorner.Y + radius}, 1.5f * core::PI, 2.0f * core::PI);
	// Bottom-right
	add_arc({rect.LowerRightCorner.X - radius, rect.LowerRightCorner.Y - radius}, 0.0f, 0.5f * core::PI);
	// Bottom-left
	add_arc({rect.UpperLeftCorner.X + radius, rect.LowerRightCorner.Y - radius}, 0.5f * core::PI, core::PI);

	for (u16 i = 1; i < vertices.size() - 1; ++i) {
		indices.push_back(0);
		indices.push_back(i);
		indices.push_back(i + 1);
	}
	// Close the loop
	indices.push_back(0);
	indices.push_back(vertices.size() - 1);
	indices.push_back(1);

	driver->draw2DVertexPrimitiveList(vertices.data(), vertices.size(),
			indices.data(), indices.size() / 3, video::EVT_STANDARD,
			scene::EPT_TRIANGLES, video::EIT_16BIT);
}

} // namespace gui
