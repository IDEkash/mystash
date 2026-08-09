// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "GUIDrawPoint.h"
#include "guiFormSpecMenu.h"
#include "util/string.h"
#include "porting.h"
#include <algorithm>
#include <cmath>

GUIDrawPoint::GUIDrawPoint(gui::IGUIEnvironment *env, gui::IGUIElement *parent, s32 id,
	GUIFormSpecMenu *menu,
	const std::string &name,
	const std::vector<v2s32> &points,
	const std::string &fill_type,
	const std::string &fill_value,
	video::ITexture *texture,
	float radius,
	const std::string &properties) :
	gui::IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, core::rect<s32>(0, 0, 1, 1)),
	m_menu(menu),
	m_name(name),
	m_orig_points(points),
	m_fill_type(fill_type),
	m_fill_value(fill_value),
	m_texture(texture),
	m_radius(radius),
	m_properties(properties)
{
	// Parse properties
	m_pressable = (properties.find("pressable=true") != std::string::npos || properties.find("pressable") != std::string::npos);
	m_hold = (properties.find("hold=true") != std::string::npos || properties.find("hold") != std::string::npos);
	m_release = (properties.find("release=true") != std::string::npos || properties.find("release") != std::string::npos);
	m_input_enabled = (properties.find("input=true") != std::string::npos || properties.find("input") != std::string::npos);

	// Default color
	m_color = video::SColor(255, 255, 255, 255);
	if (m_fill_type == "color" && !m_fill_value.empty()) {
		parseColorString(m_fill_value, m_color, true, 255);
	}

	// Generate rounded points and set relative rectangle
	generateRoundedPoints();
}

void GUIDrawPoint::generateRoundedPoints()
{
	if (m_orig_points.empty())
		return;

	// Compute bounding box of original points to set relative rectangle
	v2s32 min_pt = m_orig_points[0];
	v2s32 max_pt = m_orig_points[0];
	for (const auto &p : m_orig_points) {
		if (p.X < min_pt.X) min_pt.X = p.X;
		if (p.Y < min_pt.Y) min_pt.Y = p.Y;
		if (p.X > max_pt.X) max_pt.X = p.X;
		if (p.Y > max_pt.Y) max_pt.Y = p.Y;
	}

	// Ensure bounding box is at least 1x1
	if (min_pt.X == max_pt.X) max_pt.X = min_pt.X + 1;
	if (min_pt.Y == max_pt.Y) max_pt.Y = min_pt.Y + 1;

	// Set relative position and size of the element
	core::rect<s32> bbox(min_pt, max_pt);
	setRelativePosition(bbox);

	// Transform points to be relative to the UpperLeftCorner of bbox
	std::vector<v2s32> rel_orig;
	for (const auto &p : m_orig_points) {
		rel_orig.push_back(p - bbox.UpperLeftCorner);
	}

	if (rel_orig.size() < 3 || m_radius <= 0.0f) {
		m_rounded_points = rel_orig;
		return;
	}

	m_rounded_points.clear();
	size_t n = rel_orig.size();

	for (size_t i = 0; i < n; ++i) {
		v2s32 B = rel_orig[i];
		v2s32 A = rel_orig[(i + n - 1) % n];
		v2s32 C = rel_orig[(i + 1) % n];

		v2f32 v1(A.X - B.X, A.Y - B.Y);
		v2f32 v2(C.X - B.X, C.Y - B.Y);

		float len1 = v1.getLength();
		float len2 = v2.getLength();

		if (len1 < 0.001f || len2 < 0.001f) {
			m_rounded_points.push_back(B);
			continue;
		}

		v2f32 hat1 = v1 / len1;
		v2f32 hat2 = v2 / len2;

		// Clamp radius to half the edge length to prevent overshooting
		float r = std::min(m_radius, std::min(len1 / 2.0f, len2 / 2.0f));

		if (r <= 0.0f) {
			m_rounded_points.push_back(B);
			continue;
		}

		v2f32 Q1 = v2f32(B.X, B.Y) + hat1 * r;
		v2f32 Q2 = v2f32(B.X, B.Y) + hat2 * r;

		const int steps = 8;
		for (int s = 0; s <= steps; ++s) {
			float t = (float)s / steps;
			float omt = 1.0f - t;
			v2f32 P = Q1 * (omt * omt) + v2f32(B.X, B.Y) * (2.0f * omt * t) + Q2 * (t * t);
			m_rounded_points.push_back(v2s32(std::round(P.X), std::round(P.Y)));
		}
	}
}

void GUIDrawPoint::draw()
{
	if (!IsVisible || m_rounded_points.empty())
		return;

	video::IVideoDriver *driver = Environment->getVideoDriver();
	v2s32 upper_left = AbsoluteRect.UpperLeftCorner;

	// Draw filled polygon inside if not fill_type none
	if (m_fill_type != "none") {
		// Calculate bounding box of relative rounded points
		v2s32 min_pt = m_rounded_points[0];
		v2s32 max_pt = m_rounded_points[0];
		for (const auto &p : m_rounded_points) {
			if (p.X < min_pt.X) min_pt.X = p.X;
			if (p.Y < min_pt.Y) min_pt.Y = p.Y;
			if (p.X > max_pt.X) max_pt.X = p.X;
			if (p.Y > max_pt.Y) max_pt.Y = p.Y;
		}
		if (min_pt.X == max_pt.X) max_pt.X = min_pt.X + 1;
		if (min_pt.Y == max_pt.Y) max_pt.Y = min_pt.Y + 1;

		float bbox_w = max_pt.X - min_pt.X;
		float bbox_h = max_pt.Y - min_pt.Y;

		// Compute centroid
		v2f32 centroid(0.0f, 0.0f);
		for (const auto &p : m_rounded_points) {
			centroid.X += p.X;
			centroid.Y += p.Y;
		}
		centroid.X /= m_rounded_points.size();
		centroid.Y /= m_rounded_points.size();

		// Construct vertices
		std::vector<video::S3DVertex> vertices;
		// Vertex 0: centroid
		float u_c = (centroid.X - min_pt.X) / bbox_w;
		float v_c = (centroid.Y - min_pt.Y) / bbox_h;
		vertices.push_back(video::S3DVertex(
			upper_left.X + centroid.X, upper_left.Y + centroid.Y, 0.0f,
			0.0f, 0.0f, -1.0f, m_color, u_c, v_c
		));

		// Vertices 1..k
		for (const auto &p : m_rounded_points) {
			float u = (p.X - min_pt.X) / bbox_w;
			float v = (p.Y - min_pt.Y) / bbox_h;
			vertices.push_back(video::S3DVertex(
				upper_left.X + p.X, upper_left.Y + p.Y, 0.0f,
				0.0f, 0.0f, -1.0f, m_color, u, v
			));
		}

		// Build indices for triangles
		std::vector<u16> indices;
		u32 k = m_rounded_points.size();
		for (u32 i = 1; i < k; ++i) {
			indices.push_back(0);
			indices.push_back(i);
			indices.push_back(i + 1);
		}
		indices.push_back(0);
		indices.push_back(k);
		indices.push_back(1);

		// Set material
		video::SMaterial material;
		material.ZWriteEnable = video::EZW_OFF;
		material.ZBuffer = video::ECFN_DISABLED;
		material.BackfaceCulling = false;
		material.AntiAliasing = video::EAAM_OFF;
		material.setTexture(0, m_texture);
		if (m_texture) {
			material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
		} else {
			material.MaterialType = video::EMT_TRANSPARENT_VERTEX_ALPHA;
		}

		driver->setMaterial(material);
		driver->draw2DVertexPrimitiveList(
			vertices.data(), vertices.size(),
			indices.data(), indices.size() / 3,
			video::EVT_STANDARD, scene::EPT_TRIANGLES, video::EIT_16BIT
		);
	}

	// Draw outline/border lines connecting the points
	for (size_t i = 0; i < m_rounded_points.size(); ++i) {
		driver->draw2DLine(
			upper_left + m_rounded_points[i],
			upper_left + m_rounded_points[(i + 1) % m_rounded_points.size()],
			m_color
		);
	}

	// Draw text if textbox/input enabled
	if (m_input_enabled) {
		std::wstring display_text = m_text;
		if (Environment->getFocus() == this) {
			u32 now = porting::getTimeMs();
			if ((now / 500) % 2 == 0) {
				display_text += L"|";
			}
		}

		if (!display_text.empty()) {
			gui::IGUISkin *skin = Environment->getSkin();
			gui::IGUIFont *font = skin->getFont();
			if (font) {
				// Find size of text to center it
				core::dimension2d<u32> size = font->getDimension(display_text.c_str());
				v2s32 text_pos(
					upper_left.X + (AbsoluteRect.getWidth() - (s32)size.Width) / 2,
					upper_left.Y + (AbsoluteRect.getHeight() - (s32)size.Height) / 2
				);
				font->draw(display_text.c_str(), core::rect<s32>(text_pos, text_pos + v2s32(size.Width, size.Height)), video::SColor(255, 255, 255, 255));
			}
		}
	}

	gui::IGUIElement::draw();
}

bool GUIDrawPoint::isPointInside(const v2s32 &pt) const
{
	if (m_rounded_points.size() < 3)
		return false;

	bool inside = false;
	int n = m_rounded_points.size();
	for (int i = 0, j = n - 1; i < n; j = i++) {
		if (((m_rounded_points[i].Y > pt.Y) != (m_rounded_points[j].Y > pt.Y)) &&
			(pt.X < (m_rounded_points[j].X - m_rounded_points[i].X) * (pt.Y - m_rounded_points[i].Y) / (float)(m_rounded_points[j].Y - m_rounded_points[i].Y) + m_rounded_points[i].X)) {
			inside = !inside;
		}
	}
	return inside;
}

bool GUIDrawPoint::OnEvent(const SEvent &event)
{
	if (!IsVisible || !IsEnabled)
		return gui::IGUIElement::OnEvent(event);

	// Handle Mouse Input
	if (event.EventType == EET_MOUSE_INPUT_EVENT) {
		v2s32 mouse_pos(event.MouseInput.X, event.MouseInput.Y);
		v2s32 rel_pos = mouse_pos - AbsoluteRect.UpperLeftCorner;

		switch (event.MouseInput.Event) {
		case EMIE_LMOUSE_PRESSED_DOWN:
			if (isPointInside(rel_pos)) {
				m_is_held = true;
				if (m_input_enabled) {
					Environment->setFocus(this);
				}
				if (m_pressable) {
					if (!m_input_enabled) {
						m_text = L"press";
					}
					for (auto &s : m_menu->m_fields) {
						if (s.fid == getID()) {
							s.send = true;
							break;
						}
					}
					m_menu->acceptInput(quit_mode_no);
					for (auto &s : m_menu->m_fields) {
						if (s.fid == getID()) {
							s.send = false;
							break;
						}
					}
				}
				return true;
			}
			break;

		case EMIE_LMOUSE_LEFT_UP:
			if (m_is_held) {
				m_is_held = false;
				if (m_release && isPointInside(rel_pos)) {
					if (!m_input_enabled) {
						m_text = L"release";
					}
					for (auto &s : m_menu->m_fields) {
						if (s.fid == getID()) {
							s.send = true;
							break;
						}
					}
					m_menu->acceptInput(quit_mode_no);
					for (auto &s : m_menu->m_fields) {
						if (s.fid == getID()) {
							s.send = false;
							break;
						}
					}
				}
				return true;
			}
			break;

		case EMIE_MOUSE_MOVED:
			if (m_is_held && m_hold) {
				if (isPointInside(rel_pos)) {
					if (!m_input_enabled) {
						m_text = L"hold";
					}
					for (auto &s : m_menu->m_fields) {
						if (s.fid == getID()) {
							s.send = true;
							break;
						}
					}
					m_menu->acceptInput(quit_mode_no);
					for (auto &s : m_menu->m_fields) {
						if (s.fid == getID()) {
							s.send = false;
							break;
						}
					}
				}
				return true;
			}
			break;

		default:
			break;
		}
	}

	// Handle Touch Input
	if (event.EventType == EET_TOUCH_INPUT_EVENT) {
		v2s32 touch_pos(event.TouchInput.X, event.TouchInput.Y);
		v2s32 rel_pos = touch_pos - AbsoluteRect.UpperLeftCorner;

		switch (event.TouchInput.Event) {
		case ETIE_PRESSED_DOWN:
			if (isPointInside(rel_pos)) {
				m_is_held = true;
				if (m_input_enabled) {
					Environment->setFocus(this);
				}
				if (m_pressable) {
					if (!m_input_enabled) {
						m_text = L"press";
					}
					for (auto &s : m_menu->m_fields) {
						if (s.fid == getID()) {
							s.send = true;
							break;
						}
					}
					m_menu->acceptInput(quit_mode_no);
					for (auto &s : m_menu->m_fields) {
						if (s.fid == getID()) {
							s.send = false;
							break;
						}
					}
				}
				return true;
			}
			break;

		case ETIE_LEFT_UP:
			if (m_is_held) {
				m_is_held = false;
				if (m_release && isPointInside(rel_pos)) {
					if (!m_input_enabled) {
						m_text = L"release";
					}
					for (auto &s : m_menu->m_fields) {
						if (s.fid == getID()) {
							s.send = true;
							break;
						}
					}
					m_menu->acceptInput(quit_mode_no);
					for (auto &s : m_menu->m_fields) {
						if (s.fid == getID()) {
							s.send = false;
							break;
						}
					}
				}
				return true;
			}
			break;

		case ETIE_MOVED:
			if (m_is_held && m_hold) {
				if (isPointInside(rel_pos)) {
					if (!m_input_enabled) {
						m_text = L"hold";
					}
					for (auto &s : m_menu->m_fields) {
						if (s.fid == getID()) {
							s.send = true;
							break;
						}
					}
					m_menu->acceptInput(quit_mode_no);
					for (auto &s : m_menu->m_fields) {
						if (s.fid == getID()) {
							s.send = false;
							break;
						}
					}
				}
				return true;
			}
			break;

		default:
			break;
		}
	}

	// Handle Key Input for TextBox / Typing
	if (event.EventType == EET_KEY_INPUT_EVENT && event.KeyInput.PressedDown && m_input_enabled && Environment->getFocus() == this) {
		if (event.KeyInput.Key == KEY_BACK) {
			if (!m_text.empty()) {
				m_text.pop_back();
				for (auto &s : m_menu->m_fields) {
					if (s.fid == getID()) {
						s.send = true;
						break;
					}
				}
				m_menu->acceptInput(quit_mode_no);
				for (auto &s : m_menu->m_fields) {
					if (s.fid == getID()) {
						s.send = false;
						break;
					}
				}
				return true;
			}
		} else if (event.KeyInput.Char >= 32) {
			m_text.push_back(event.KeyInput.Char);
			for (auto &s : m_menu->m_fields) {
				if (s.fid == getID()) {
					s.send = true;
					break;
				}
			}
			m_menu->acceptInput(quit_mode_no);
			for (auto &s : m_menu->m_fields) {
				if (s.fid == getID()) {
					s.send = false;
					break;
				}
			}
			return true;
		}
	}

	return gui::IGUIElement::OnEvent(event);
}

const wchar_t *GUIDrawPoint::getText() const
{
	return m_text.c_str();
}
