// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "guiutil.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include "irrMath.h"
#include "IVideoDriver.h"
#include "S3DVertex.h"

namespace gui {

void draw2DRoundedRectangle(video::IVideoDriver *driver,
		const core::rect<s32> &rect, video::SColor color,
		s32 radius, const core::rect<s32> *clip,
		video::SColor border_color, s32 border_width)
{
	if (!driver)
		return;

	if (radius <= 0) {
		driver->draw2DRectangle(color, rect, clip);
		if (border_width > 0 && border_color.getAlpha() > 0) {
			driver->draw2DLine({rect.UpperLeftCorner.X, rect.UpperLeftCorner.Y}, {rect.LowerRightCorner.X, rect.UpperLeftCorner.Y}, border_color);
			driver->draw2DLine({rect.LowerRightCorner.X, rect.UpperLeftCorner.Y}, {rect.LowerRightCorner.X, rect.LowerRightCorner.Y}, border_color);
			driver->draw2DLine({rect.LowerRightCorner.X, rect.LowerRightCorner.Y}, {rect.UpperLeftCorner.X, rect.LowerRightCorner.Y}, border_color);
			driver->draw2DLine({rect.UpperLeftCorner.X, rect.LowerRightCorner.Y}, {rect.UpperLeftCorner.X, rect.UpperLeftCorner.Y}, border_color);
		}
		return;
	}

	radius = std::min({radius, rect.getWidth() / 2, rect.getHeight() / 2});

	auto draw_poly = [&](video::SColor c, s32 r, const core::rect<s32>& box) {
		std::vector<video::S3DVertex> vertices;
		std::vector<u16> indices;

		auto add_arc = [&](core::vector2di center, float start_angle, float end_angle) {
			const int segments = 8;
			for (int i = 0; i <= segments; ++i) {
				float angle = start_angle + (end_angle - start_angle) * i / segments;
				float x = center.X + r * std::cos(angle);
				float y = center.Y + r * std::sin(angle);
				vertices.emplace_back(x, y, 0, 0, 0, 0, c, 0, 0);
			}
		};

		vertices.emplace_back(box.getCenter().X, box.getCenter().Y, 0, 0, 0, 0, c, 0, 0);
		add_arc({box.UpperLeftCorner.X + r, box.UpperLeftCorner.Y + r}, core::PI, 1.5f * core::PI);
		add_arc({box.LowerRightCorner.X - r, box.UpperLeftCorner.Y + r}, 1.5f * core::PI, 2.0f * core::PI);
		add_arc({box.LowerRightCorner.X - r, box.LowerRightCorner.Y - r}, 0.0f, 0.5f * core::PI);
		add_arc({box.UpperLeftCorner.X + r, box.LowerRightCorner.Y - r}, 0.5f * core::PI, core::PI);

		for (u16 i = 1; i < vertices.size() - 1; ++i) {
			indices.push_back(0);
			indices.push_back(i);
			indices.push_back(i + 1);
		}
		indices.push_back(0);
		indices.push_back(vertices.size() - 1);
		indices.push_back(1);

		driver->draw2DVertexPrimitiveList(vertices.data(), vertices.size(),
				indices.data(), indices.size() / 3, video::EVT_STANDARD,
				scene::EPT_TRIANGLES, video::EIT_16BIT);
	};

	if (border_width > 0 && border_color.getAlpha() > 0) {
		draw_poly(border_color, radius, rect);
		core::rect<s32> inner_rect = rect;
		inner_rect.UpperLeftCorner += border_width;
		inner_rect.LowerRightCorner -= border_width;
		draw_poly(color, std::max(0, radius - border_width), inner_rect);
	} else {
		draw_poly(color, radius, rect);
	}
}

} // namespace gui
