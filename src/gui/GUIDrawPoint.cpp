// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "GUIDrawPoint.h"
#include "guiFormSpecMenu.h"
#include "util/string.h"
#include "porting.h"
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Define static registry for safe casting
std::unordered_set<gui::IGUIElement*> GUIDrawPoint::s_active_drawpoints;

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
	// Insert ourselves into the safe static registry
	s_active_drawpoints.insert(this);

	// Parse properties
	m_pressable = (properties.find("pressable=true") != std::string::npos || properties.find("pressable") != std::string::npos);
	m_hold = (properties.find("hold=true") != std::string::npos || properties.find("hold") != std::string::npos);
	m_release = (properties.find("release=true") != std::string::npos || properties.find("release") != std::string::npos);
	m_input_enabled = (properties.find("input=true") != std::string::npos || properties.find("input") != std::string::npos);
	m_draggable = (properties.find("draggable=true") != std::string::npos || properties.find("draggable") != std::string::npos);

	// Parse parenting if present
	size_t parent_pos = properties.find("parent=");
	if (parent_pos != std::string::npos) {
		size_t comma_pos = properties.find(",", parent_pos);
		if (comma_pos == std::string::npos) {
			m_parent_name = properties.substr(parent_pos + 7);
		} else {
			m_parent_name = properties.substr(parent_pos + 7, comma_pos - (parent_pos + 7));
		}
	}

	// Parse custom label text if defined
	size_t text_pos = properties.find("text=");
	if (text_pos != std::string::npos) {
		size_t comma_pos = properties.find(",", text_pos);
		std::string raw_text;
		if (comma_pos == std::string::npos) {
			raw_text = properties.substr(text_pos + 5);
		} else {
			raw_text = properties.substr(text_pos + 5, comma_pos - (text_pos + 5));
		}
		m_label_text = utf8_to_wide(unescape_string(raw_text));
	}

	// Parse runtime transform properties if defined with robust try-catch wrapper
	try {
		size_t px_pos = properties.find("px=");
		size_t py_pos = properties.find("py=");
		if (px_pos != std::string::npos && py_pos != std::string::npos) {
			m_pos_offset.X = stoi(properties.substr(px_pos + 3));
			m_pos_offset.Y = stoi(properties.substr(py_pos + 3));
		}
	} catch (...) {
		m_pos_offset = v2s32(0, 0);
	}

	try {
		size_t sx_pos = properties.find("sx=");
		size_t sy_pos = properties.find("sy=");
		if (sx_pos != std::string::npos && sy_pos != std::string::npos) {
			m_scale.X = stof(properties.substr(sx_pos + 3));
			m_scale.Y = stof(properties.substr(sy_pos + 3));
		}
	} catch (...) {
		m_scale = v2f32(1.0f, 1.0f);
	}

	try {
		size_t rot_pos = properties.find("rot=");
		if (rot_pos != std::string::npos) {
			m_rotation = stof(properties.substr(rot_pos + 4));
		}
	} catch (...) {
		m_rotation = 0.0f;
	}

	// Default color
	m_color = video::SColor(255, 255, 255, 255);
	if (m_fill_type == "color" && !m_fill_value.empty()) {
		parseColorString(m_fill_value, m_color, true, 255);
	}

	// Check if animation duration is specified
	size_t anim_pos = properties.find("anim_duration=");
	if (anim_pos != std::string::npos) {
		try {
			u32 duration = std::stoul(properties.substr(anim_pos + 14));
			if (duration > 0) {
				// Trigger initial transition animation from zero scale / center pos to target
				v2s32 min_pt = m_orig_points[0];
				v2s32 max_pt = m_orig_points[0];
				for (const auto &p : m_orig_points) {
					if (p.X < min_pt.X) min_pt.X = p.X;
					if (p.Y < min_pt.Y) min_pt.Y = p.Y;
					if (p.X > max_pt.X) max_pt.X = p.X;
					if (p.Y > max_pt.Y) max_pt.Y = p.Y;
				}
				core::rect<s32> bbox(min_pt, max_pt);
				v2s32 center = bbox.getCenter();

				// Save targets
				v2s32 target_pos = m_pos_offset;
				v2f32 target_scale = m_scale;
				float target_rot = m_rotation;
				video::SColor target_col = m_color;

				// Set starting values before animation trigger
				m_pos_offset = center - bbox.UpperLeftCorner;
				m_scale = v2f32(0.0f, 0.0f);

				startAnimation(target_pos, target_scale, target_rot, target_col, duration);
			}
		} catch (...) {
			// Ignore anim parsing errors gracefully
		}
	}

	// Generate rounded points and set relative rectangle
	generateRoundedPoints();
}

GUIDrawPoint::~GUIDrawPoint()
{
	// Remove ourselves from thesafe static registry
	s_active_drawpoints.erase(this);
}

std::vector<v2s32> GUIDrawPoint::calculateRoundedPoints(
	const std::vector<v2s32> &orig_points, float radius)
{
	if (orig_points.size() < 3 || radius <= 0.0f) {
		return orig_points;
	}

	std::vector<v2s32> rounded_points;
	size_t n = orig_points.size();

	for (size_t i = 0; i < n; ++i) {
		v2s32 B = orig_points[i];
		v2s32 A = orig_points[(i + n - 1) % n];
		v2s32 C = orig_points[(i + 1) % n];

		v2f32 v1(A.X - B.X, A.Y - B.Y);
		v2f32 v2(C.X - B.X, C.Y - B.Y);

		float len1 = v1.getLength();
		float len2 = v2.getLength();

		if (len1 < 0.001f || len2 < 0.001f) {
			rounded_points.push_back(B);
			continue;
		}

		v2f32 hat1 = v1 / len1;
		v2f32 hat2 = v2 / len2;

		// Clamp radius to half the edge length to prevent overshooting
		float r = std::min(radius, std::min(len1 / 2.0f, len2 / 2.0f));

		if (r <= 0.0f) {
			rounded_points.push_back(B);
			continue;
		}

		v2f32 Q1 = v2f32(B.X, B.Y) + hat1 * r;
		v2f32 Q2 = v2f32(B.X, B.Y) + hat2 * r;

		const int steps = 8;
		for (int s = 0; s <= steps; ++s) {
			float t = (float)s / steps;
			float omt = 1.0f - t;
			v2f32 P = Q1 * (omt * omt) + v2f32(B.X, B.Y) * (2.0f * omt * t) + Q2 * (t * t);
			rounded_points.push_back(v2s32(std::round(P.X), std::round(P.Y)));
		}
	}
	return rounded_points;
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

	m_rounded_points = calculateRoundedPoints(rel_orig, m_radius);
}

std::vector<v2s32> GUIDrawPoint::getAbsolutePoints() const
{
	return getAbsolutePointsRecursive(0);
}

std::vector<v2s32> GUIDrawPoint::getAbsolutePointsRecursive(int depth) const
{
	if (m_rounded_points.empty())
		return m_rounded_points;

	if (depth > 16) {
		// Safe-guard to prevent stack overflow on circular parenting loops
		return m_rounded_points;
	}

	v2s32 upper_left = AbsoluteRect.UpperLeftCorner;

	// 1. Calculate centroid of local rounded points
	v2f32 centroid(0.0f, 0.0f);
	for (const auto &p : m_rounded_points) {
		centroid.X += p.X;
		centroid.Y += p.Y;
	}
	centroid.X /= m_rounded_points.size();
	centroid.Y /= m_rounded_points.size();

	// 2. Apply local transforms
	std::vector<v2s32> transformed;
	for (const auto &p : m_rounded_points) {
		v2f32 local_p(p.X - centroid.X, p.Y - centroid.Y);
		// Scale
		local_p.X *= m_scale.X;
		local_p.Y *= m_scale.Y;
		// Rotate
		if (m_rotation != 0.0f) {
			float rad = m_rotation * M_PI / 180.0f;
			float cos_r = std::cos(rad);
			float sin_r = std::sin(rad);
			float rx = local_p.X * cos_r - local_p.Y * sin_r;
			float ry = local_p.X * sin_r + local_p.Y * cos_r;
			local_p.X = rx;
			local_p.Y = ry;
		}
		// local offset + world position
		v2s32 abs_p(
			std::round(local_p.X + centroid.X) + m_pos_offset.X + upper_left.X,
			std::round(local_p.Y + centroid.Y) + m_pos_offset.Y + upper_left.Y
		);
		transformed.push_back(abs_p);
	}

	// 3. Recursive parenting (applying parent transform to child absolute coordinates)
	if (!m_parent_name.empty()) {
		GUIDrawPoint *parent_dp = m_menu->getDrawPointByName(m_parent_name);
		if (parent_dp && !parent_dp->m_rounded_points.empty()) {
			std::vector<v2s32> parent_pts = parent_dp->getAbsolutePointsRecursive(depth + 1);
			if (!parent_pts.empty()) {
				// 1. Compute parent's untransformed centroid in absolute world space
				v2f32 p_centroid_local(0.0f, 0.0f);
				for (const auto &p : parent_dp->m_rounded_points) {
					p_centroid_local.X += p.X;
					p_centroid_local.Y += p.Y;
				}
				p_centroid_local.X /= parent_dp->m_rounded_points.size();
				p_centroid_local.Y /= parent_dp->m_rounded_points.size();

				v2f32 p_centroid_untransformed = v2f32(parent_dp->AbsoluteRect.UpperLeftCorner.X, parent_dp->AbsoluteRect.UpperLeftCorner.Y) + p_centroid_local;

				// 2. Compute parent's recursively transformed world centroid
				v2f32 p_centroid_world(0.0f, 0.0f);
				for (const auto &p : parent_pts) {
					p_centroid_world.X += p.X;
					p_centroid_world.Y += p.Y;
				}
				p_centroid_world.X /= parent_pts.size();
				p_centroid_world.Y /= parent_pts.size();

				for (auto &pt : transformed) {
					v2f32 rel_p(pt.X - p_centroid_untransformed.X, pt.Y - p_centroid_untransformed.Y);
					// Scale by parent
					rel_p.X *= parent_dp->m_scale.X;
					rel_p.Y *= parent_dp->m_scale.Y;
					// Rotate by parent
					if (parent_dp->m_rotation != 0.0f) {
						float rad = parent_dp->m_rotation * M_PI / 180.0f;
						float cos_r = std::cos(rad);
						float sin_r = std::sin(rad);
						float rx = rel_p.X * cos_r - rel_p.Y * sin_r;
						float ry = rel_p.X * sin_r + rel_p.Y * cos_r;
						rel_p.X = rx;
						rel_p.Y = ry;
					}
					// Project to parent's world space
					pt = v2s32(
						std::round(rel_p.X + p_centroid_world.X),
						std::round(rel_p.Y + p_centroid_world.Y)
					);
				}
			}
		}
	}

	return transformed;
}

video::SColor GUIDrawPoint::getInheritedColor() const
{
	video::SColor col = m_color;
	if (!m_parent_name.empty()) {
		GUIDrawPoint *parent_dp = m_menu->getDrawPointByName(m_parent_name);
		if (parent_dp) {
			video::SColor parent_col = parent_dp->getInheritedColor();
			col.setAlpha((col.getAlpha() * parent_col.getAlpha()) / 255);
			col.setRed((col.getRed() * parent_col.getRed()) / 255);
			col.setGreen((col.getGreen() * parent_col.getGreen()) / 255);
			col.setBlue((col.getBlue() * parent_col.getBlue()) / 255);
		}
	}
	return col;
}

void GUIDrawPoint::draw()
{
	if (!IsVisible || m_rounded_points.empty())
		return;

	video::IVideoDriver *driver = Environment->getVideoDriver();
	u32 now = porting::getTimeMs();

	// Process animation interpolation
	if (m_anim.active) {
		if (now >= m_anim.start_time + m_anim.duration) {
			m_pos_offset = m_anim.target_pos;
			m_scale = m_anim.target_scale;
			m_rotation = m_anim.target_rotation;
			m_color = m_anim.target_color;
			m_anim.active = false;
		} else {
			float t = (float)(now - m_anim.start_time) / m_anim.duration;
			t = t * t * (3.0f - 2.0f * t); // Smoothstep easing

			m_pos_offset.X = m_anim.start_pos.X + t * (m_anim.target_pos.X - m_anim.start_pos.X);
			m_pos_offset.Y = m_anim.start_pos.Y + t * (m_anim.target_pos.Y - m_anim.start_pos.Y);

			m_scale.X = m_anim.start_scale.X + t * (m_anim.target_scale.X - m_anim.start_scale.X);
			m_scale.Y = m_anim.start_scale.Y + t * (m_anim.target_scale.Y - m_anim.start_scale.Y);

			m_rotation = m_anim.start_rotation + t * (m_anim.target_rotation - m_anim.start_rotation);

			m_color.setRed(m_anim.start_color.getRed() + t * ((int)m_anim.target_color.getRed() - m_anim.start_color.getRed()));
			m_color.setGreen(m_anim.start_color.getGreen() + t * ((int)m_anim.target_color.getGreen() - m_anim.start_color.getGreen()));
			m_color.setBlue(m_anim.start_color.getBlue() + t * ((int)m_anim.target_color.getBlue() - m_anim.start_color.getBlue()));
			m_color.setAlpha(m_anim.start_color.getAlpha() + t * ((int)m_anim.target_color.getAlpha() - m_anim.start_color.getAlpha()));
		}
	}

	std::vector<v2s32> abs_points = getAbsolutePoints();
	if (abs_points.empty())
		return;

	// Compute fully inherited cascading color/alpha
	video::SColor draw_color = getInheritedColor();

	// Draw filled polygon inside if not fill_type none
	if (m_fill_type != "none") {
		v2s32 min_pt = abs_points[0];
		v2s32 max_pt = abs_points[0];
		for (const auto &p : abs_points) {
			if (p.X < min_pt.X) min_pt.X = p.X;
			if (p.Y < min_pt.Y) min_pt.Y = p.Y;
			if (p.X > max_pt.X) max_pt.X = p.X;
			if (p.Y > max_pt.Y) max_pt.Y = p.Y;
		}
		if (min_pt.X == max_pt.X) max_pt.X = min_pt.X + 1;
		if (min_pt.Y == max_pt.Y) max_pt.Y = min_pt.Y + 1;

		float bbox_w = max_pt.X - min_pt.X;
		float bbox_h = max_pt.Y - min_pt.Y;

		v2f32 centroid(0.0f, 0.0f);
		for (const auto &p : abs_points) {
			centroid.X += p.X;
			centroid.Y += p.Y;
		}
		centroid.X /= abs_points.size();
		centroid.Y /= abs_points.size();

		std::vector<video::S3DVertex> vertices;
		float u_c = (centroid.X - min_pt.X) / bbox_w;
		float v_c = (centroid.Y - min_pt.Y) / bbox_h;
		vertices.push_back(video::S3DVertex(
			centroid.X, centroid.Y, 0.0f,
			0.0f, 0.0f, -1.0f, draw_color, u_c, v_c
		));

		for (const auto &p : abs_points) {
			float u = (p.X - min_pt.X) / bbox_w;
			float v = (p.Y - min_pt.Y) / bbox_h;
			vertices.push_back(video::S3DVertex(
				p.X, p.Y, 0.0f,
				0.0f, 0.0f, -1.0f, draw_color, u, v
			));
		}

		std::vector<u16> indices;
		u32 k = abs_points.size();
		for (u32 i = 1; i < k; ++i) {
			indices.push_back(0);
			indices.push_back(i);
			indices.push_back(i + 1);
		}
		indices.push_back(0);
		indices.push_back(k);
		indices.push_back(1);

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
	for (size_t i = 0; i < abs_points.size(); ++i) {
		driver->draw2DLine(
			abs_points[i],
			abs_points[(i + 1) % abs_points.size()],
			draw_color
		);
	}

	// Draw custom label text or typed input text if enabled
	std::wstring display_text = m_label_text;
	if (m_input_enabled) {
		display_text = m_text;
		if (Environment->getFocus() == this) {
			if ((now / 500) % 2 == 0) {
				display_text += L"|";
			}
		}
	}

	if (!display_text.empty()) {
		gui::IGUISkin *skin = Environment->getSkin();
		gui::IGUIFont *font = skin->getFont();
		if (font) {
			core::dimension2d<u32> size = font->getDimension(display_text.c_str());
			v2s32 text_pos(
				AbsoluteRect.UpperLeftCorner.X + (AbsoluteRect.getWidth() - (s32)size.Width) / 2,
				AbsoluteRect.UpperLeftCorner.Y + (AbsoluteRect.getHeight() - (s32)size.Height) / 2
			);
			font->draw(display_text.c_str(), core::rect<s32>(text_pos, text_pos + v2s32(size.Width, size.Height)), video::SColor(255, 255, 255, 255));
		}
	}

	gui::IGUIElement::draw();
}

bool GUIDrawPoint::isPointInsidePolygon(const v2s32 &pt, const std::vector<v2s32> &poly)
{
	if (poly.size() < 3)
		return false;

	bool inside = false;
	int n = poly.size();
	for (int i = 0, j = n - 1; i < n; j = i++) {
		if (((poly[i].Y > pt.Y) != (poly[j].Y > pt.Y)) &&
			(pt.X < (poly[j].X - poly[i].X) * (pt.Y - poly[i].Y) / (float)(poly[j].Y - poly[i].Y) + poly[i].X)) {
			inside = !inside;
		}
	}
	return inside;
}

bool GUIDrawPoint::isPointInside(const v2s32 &pt) const
{
	return isPointInsidePolygon(pt, getAbsolutePoints());
}

bool GUIDrawPoint::OnEvent(const SEvent &event)
{
	if (!IsVisible || !IsEnabled)
		return gui::IGUIElement::OnEvent(event);

	// Handle Mouse Input
	if (event.EventType == EET_MOUSE_INPUT_EVENT) {
		v2s32 mouse_pos(event.MouseInput.X, event.MouseInput.Y);

		switch (event.MouseInput.Event) {
		case EMIE_LMOUSE_PRESSED_DOWN:
			if (isPointInside(mouse_pos)) {
				m_is_held = true;
				if (m_input_enabled) {
					Environment->setFocus(this);
				}
				if (m_draggable) {
					m_is_dragging = true;
					m_drag_start_pointer = mouse_pos;
					m_drag_start_offset = m_pos_offset;
					m_text = L"drag_start";
					for (auto &s : m_menu->m_fields) {
						if (s.fid == getID()) {
							s.send = true;
							break;
						}
					}
					m_menu->acceptInput(quit_mode_no);
					if (s_active_drawpoints.find(this) == s_active_drawpoints.end()) {
						return true; // Safely abort if deleted on formspec update
					}
					for (auto &s : m_menu->m_fields) {
						if (s.fid == getID()) {
							s.send = false;
							break;
						}
					}
				} else if (m_pressable) {
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
					if (s_active_drawpoints.find(this) == s_active_drawpoints.end()) {
						return true; // Safely abort if deleted on formspec update
					}
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
			if (m_is_dragging) {
				m_is_dragging = false;
				m_text = L"drag_end";
				for (auto &s : m_menu->m_fields) {
					if (s.fid == getID()) {
						s.send = true;
						break;
					}
				}
				m_menu->acceptInput(quit_mode_no);
				if (s_active_drawpoints.find(this) == s_active_drawpoints.end()) {
					return true; // Safely abort if deleted on formspec update
				}
				for (auto &s : m_menu->m_fields) {
					if (s.fid == getID()) {
						s.send = false;
						break;
					}
				}
				return true;
			}
			if (m_is_held) {
				m_is_held = false;
				if (m_release && isPointInside(mouse_pos)) {
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
					if (s_active_drawpoints.find(this) == s_active_drawpoints.end()) {
						return true; // Safely abort if deleted on formspec update
					}
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
			if (m_is_dragging) {
				v2s32 delta = mouse_pos - m_drag_start_pointer;
				m_pos_offset = m_drag_start_offset + delta;

				// Throttle intermediate "drag" events to once every 150ms to prevent network flooding!
				u32 now = porting::getTimeMs();
				if (now - m_last_drag_send_time >= 150) {
					m_last_drag_send_time = now;
					m_text = L"drag";
					for (auto &s : m_menu->m_fields) {
						if (s.fid == getID()) {
							s.send = true;
							break;
						}
					}
					m_menu->acceptInput(quit_mode_no);
					if (s_active_drawpoints.find(this) == s_active_drawpoints.end()) {
						return true; // Safely abort if deleted on formspec update
					}
					for (auto &s : m_menu->m_fields) {
						if (s.fid == getID()) {
							s.send = false;
							break;
						}
					}
				}
				return true;
			}
			if (m_is_held && m_hold) {
				if (isPointInside(mouse_pos)) {
					// Throttle intermediate "hold" events to once every 150ms as well!
					u32 now = porting::getTimeMs();
					if (now - m_last_hold_send_time >= 150) {
						m_last_hold_send_time = now;
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
						if (s_active_drawpoints.find(this) == s_active_drawpoints.end()) {
							return true; // Safely abort if deleted on formspec update
						}
						for (auto &s : m_menu->m_fields) {
							if (s.fid == getID()) {
								s.send = false;
								break;
							}
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

		switch (event.TouchInput.Event) {
		case ETIE_PRESSED_DOWN:
			if (isPointInside(touch_pos)) {
				m_is_held = true;
				if (m_input_enabled) {
					Environment->setFocus(this);
				}
				if (m_draggable) {
					m_is_dragging = true;
					m_drag_start_pointer = touch_pos;
					m_drag_start_offset = m_pos_offset;
					m_text = L"drag_start";
					for (auto &s : m_menu->m_fields) {
						if (s.fid == getID()) {
							s.send = true;
							break;
						}
					}
					m_menu->acceptInput(quit_mode_no);
					if (s_active_drawpoints.find(this) == s_active_drawpoints.end()) {
						return true; // Safely abort if deleted on formspec update
					}
					for (auto &s : m_menu->m_fields) {
						if (s.fid == getID()) {
							s.send = false;
							break;
						}
					}
				} else if (m_pressable) {
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
					if (s_active_drawpoints.find(this) == s_active_drawpoints.end()) {
						return true; // Safely abort if deleted on formspec update
					}
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
			if (m_is_dragging) {
				m_is_dragging = false;
				m_text = L"drag_end";
				for (auto &s : m_menu->m_fields) {
					if (s.fid == getID()) {
						s.send = true;
						break;
					}
				}
				m_menu->acceptInput(quit_mode_no);
				if (s_active_drawpoints.find(this) == s_active_drawpoints.end()) {
					return true; // Safely abort if deleted on formspec update
				}
				for (auto &s : m_menu->m_fields) {
					if (s.fid == getID()) {
						s.send = false;
						break;
					}
				}
				return true;
			}
			if (m_is_held) {
				m_is_held = false;
				if (m_release && isPointInside(touch_pos)) {
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
					if (s_active_drawpoints.find(this) == s_active_drawpoints.end()) {
						return true; // Safely abort if deleted on formspec update
					}
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
			if (m_is_dragging) {
				v2s32 delta = touch_pos - m_drag_start_pointer;
				m_pos_offset = m_drag_start_offset + delta;

				// Throttle intermediate "drag" events to once every 150ms to prevent network flooding!
				u32 now = porting::getTimeMs();
				if (now - m_last_drag_send_time >= 150) {
					m_last_drag_send_time = now;
					m_text = L"drag";
					for (auto &s : m_menu->m_fields) {
						if (s.fid == getID()) {
							s.send = true;
							break;
						}
					}
					m_menu->acceptInput(quit_mode_no);
					if (s_active_drawpoints.find(this) == s_active_drawpoints.end()) {
						return true; // Safely abort if deleted on formspec update
					}
					for (auto &s : m_menu->m_fields) {
						if (s.fid == getID()) {
							s.send = false;
							break;
						}
					}
				}
				return true;
			}
			if (m_is_held && m_hold) {
				if (isPointInside(touch_pos)) {
					// Throttle intermediate "hold" events to once every 150ms!
					u32 now = porting::getTimeMs();
					if (now - m_last_hold_send_time >= 150) {
						m_last_hold_send_time = now;
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
						if (s_active_drawpoints.find(this) == s_active_drawpoints.end()) {
							return true; // Safely abort if deleted on formspec update
						}
						for (auto &s : m_menu->m_fields) {
							if (s.fid == getID()) {
								s.send = false;
								break;
							}
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
				if (s_active_drawpoints.find(this) == s_active_drawpoints.end()) {
					return true; // Safely abort if deleted on formspec update
				}
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
			if (s_active_drawpoints.find(this) == s_active_drawpoints.end()) {
				return true; // Safely abort if deleted on formspec update
			}
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

void GUIDrawPoint::setPositionOffset(const v2s32 &offset)
{
	m_pos_offset = offset;
}

void GUIDrawPoint::setScale(const v2f32 &scale)
{
	m_scale = scale;
}

void GUIDrawPoint::setRotation(float rotation_deg)
{
	m_rotation = rotation_deg;
}

void GUIDrawPoint::setZIndex(int z)
{
	for (auto &s : m_menu->m_fields) {
		if (s.fid == getID()) {
			s.priority = z;
			break;
		}
	}
	m_menu->sortChildrenByPriorityOf(Parent);
}

void GUIDrawPoint::setParentName(const std::string &parent_name)
{
	m_parent_name = parent_name;
}

void GUIDrawPoint::startAnimation(const v2s32 &target_pos, const v2f32 &target_scale, float target_rot, video::SColor target_color, u32 duration_ms)
{
	m_anim.active = true;
	m_anim.start_time = porting::getTimeMs();
	m_anim.duration = duration_ms;

	m_anim.start_pos = m_pos_offset;
	m_anim.target_pos = target_pos;

	m_anim.start_scale = m_scale;
	m_anim.target_scale = target_scale;

	m_anim.start_rotation = m_rotation;
	m_anim.target_rotation = target_rot;

	m_anim.start_color = m_color;
	m_anim.target_color = target_color;
}
