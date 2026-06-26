// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "guiUtil.h"
#include <cmath>
#include <array>

namespace gui {

void drawRoundedRectangle(video::IVideoDriver *driver, const core::rect<s32> &rect,
		const video::SColor &color, float radius, const core::rect<s32> *clip)
{
	video::SColor colors[4] = {color, color, color, color};
	drawRoundedRectangle(driver, rect, colors, radius, clip);
}

void drawRoundedRectangle(video::IVideoDriver *driver, const core::rect<s32> &rect,
		const video::SColor colors[4], float radius, const core::rect<s32> *clip)
{
	if (radius <= 0.0f) {
		driver->draw2DRectangle(rect, colors[0], colors[1], colors[3], colors[2], clip);
		return;
	}

	float w = (float)rect.getWidth();
	float h = (float)rect.getHeight();
	radius = std::min(radius, std::min(w, h) * 0.5f);

	constexpr int segments = 8;
	constexpr int num_vertices = 1 + (segments + 1) * 4;
	constexpr int num_indices = segments * 4 * 3 + 3; // +3 to close the fan

	video::S3DVertex vertices[num_vertices];
	u16 indices[num_indices * 3]; // overkill but safe

	// Precompute sin/cos for one quadrant
	static float s_sin[segments + 1];
	static float s_cos[segments + 1];
	static bool s_initialized = false;
	if (!s_initialized) {
		for (int i = 0; i <= segments; ++i) {
			float angle = (float)i / (float)segments * (3.14159265f * 0.5f);
			s_sin[i] = std::sin(angle);
			s_cos[i] = std::cos(angle);
		}
		s_initialized = true;
	}

	// Center vertex for the fan
	vertices[0].Pos = core::vector3df((float)rect.getCenter().X, (float)rect.getCenter().Y, 0);
	vertices[0].Color = colors[0].getInterpolated(colors[1], 0.5f).getInterpolated(
		colors[2].getInterpolated(colors[3], 0.5f), 0.5f);

	float left = (float)rect.UpperLeftCorner.X;
	float top = (float)rect.UpperLeftCorner.Y;
	float right = (float)rect.LowerRightCorner.X;
	float bottom = (float)rect.LowerRightCorner.Y;

	int v_idx = 1;

	// Top-left (PI to 1.5PI)
	for (int i = 0; i <= segments; ++i) {
		vertices[v_idx].Pos = core::vector3df(left + radius - s_cos[segments - i] * radius, top + radius - s_sin[segments - i] * radius, 0);
		vertices[v_idx].Color = colors[0];
		v_idx++;
	}
	// Top-right (1.5PI to 2PI)
	for (int i = 0; i <= segments; ++i) {
		vertices[v_idx].Pos = core::vector3df(right - radius + s_sin[i] * radius, top + radius - s_cos[i] * radius, 0);
		vertices[v_idx].Color = colors[1];
		v_idx++;
	}
	// Bottom-right (0 to 0.5PI)
	for (int i = 0; i <= segments; ++i) {
		vertices[v_idx].Pos = core::vector3df(right - radius + s_cos[i] * radius, bottom - radius + s_sin[i] * radius, 0);
		vertices[v_idx].Color = colors[2];
		v_idx++;
	}
	// Bottom-left (0.5PI to PI)
	for (int i = 0; i <= segments; ++i) {
		vertices[v_idx].Pos = core::vector3df(left + radius - s_sin[i] * radius, bottom - radius + s_cos[i] * radius, 0);
		vertices[v_idx].Color = colors[3];
		v_idx++;
	}

	int i_idx = 0;
	for (int i = 1; i < v_idx - 1; ++i) {
		indices[i_idx++] = 0;
		indices[i_idx++] = i;
		indices[i_idx++] = i + 1;
	}
	// Close the fan
	indices[i_idx++] = 0;
	indices[i_idx++] = v_idx - 1;
	indices[i_idx++] = 1;

	video::SMaterial material;
	material.Lighting = false;
	material.ZWriteEnable = false;
	material.ZBuffer = video::ECFN_NEVER;
	material.BackfaceCulling = false;
	material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;

	driver->setMaterial(material);

	if (clip) {
		driver->setClipPlane(0, core::plane3df((float)clip->UpperLeftCorner.X, 0, 0, -1, 0, 0), true);
		driver->setClipPlane(1, core::plane3df((float)clip->LowerRightCorner.X, 0, 0, 1, 0, 0), true);
		driver->setClipPlane(2, core::plane3df(0, (float)clip->UpperLeftCorner.Y, 0, 0, -1, 0), true);
		driver->setClipPlane(3, core::plane3df(0, (float)clip->LowerRightCorner.Y, 0, 0, 1, 0), true);
	}

	driver->drawVertexPrimitiveList(vertices, v_idx, indices, i_idx / 3);

	if (clip) {
		for (int i = 0; i < 4; ++i)
			driver->enableClipPlane(i, false);
	}
}

} // namespace gui
