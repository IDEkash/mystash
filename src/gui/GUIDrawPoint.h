// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include <vector>
#include <string>
#include <IGUIElement.h>
#include <IGUIEnvironment.h>
#include <IVideoDriver.h>
#include "irr_v2d.h"
#include "SColor.h"

class GUIFormSpecMenu;

class GUIDrawPoint : public gui::IGUIElement
{
	friend class GUIFormSpecMenu;

public:
	struct AnimationState {
		bool active = false;
		u32 start_time = 0;
		u32 duration = 0;

		v2s32 start_pos;
		v2s32 target_pos;

		v2f32 start_scale;
		v2f32 target_scale;

		float start_rotation = 0.0f;
		float target_rotation = 0.0f;

		video::SColor start_color;
		video::SColor target_color;
	};

	GUIDrawPoint(gui::IGUIEnvironment *env, gui::IGUIElement *parent, s32 id,
		GUIFormSpecMenu *menu,
		const std::string &name,
		const std::vector<v2s32> &points,
		const std::string &fill_type,
		const std::string &fill_value,
		video::ITexture *texture,
		float radius,
		const std::string &properties);

	virtual ~GUIDrawPoint() = default;

	virtual void draw() override;
	virtual bool OnEvent(const SEvent &event) override;
	virtual const wchar_t *getText() const override;

	// Dynamic setters for modder feedback suggestions
	void setPositionOffset(const v2s32 &offset);
	void setScale(const v2f32 &scale);
	void setRotation(float rotation_deg);
	void setZIndex(int z);
	void setParentName(const std::string &parent_name);
	void startAnimation(const v2s32 &target_pos, const v2f32 &target_scale, float target_rot, video::SColor target_color, u32 duration_ms);

	std::vector<v2s32> getAbsolutePoints() const;
	std::vector<v2s32> getAbsolutePointsRecursive(int depth) const;

	// Static math helpers for production use and unit testing
	static std::vector<v2s32> calculateRoundedPoints(
		const std::vector<v2s32> &orig_points, float radius);
	static bool isPointInsidePolygon(const v2s32 &pt, const std::vector<v2s32> &poly);

private:
	void generateRoundedPoints();
	bool isPointInside(const v2s32 &pt) const;

	GUIFormSpecMenu *m_menu;
	std::string m_name;
	std::vector<v2s32> m_orig_points;
	std::vector<v2s32> m_rounded_points;
	std::string m_fill_type;
	std::string m_fill_value;
	video::ITexture *m_texture;
	float m_radius;
	std::string m_properties;

	video::SColor m_color;
	bool m_pressable = false;
	bool m_hold = false;
	bool m_release = false;
	bool m_input_enabled = false;
	bool m_draggable = false;

	std::wstring m_text;

	// Transformation and parenting states
	v2s32 m_pos_offset = v2s32(0, 0);
	v2f32 m_scale = v2f32(1.0f, 1.0f);
	float m_rotation = 0.0f;
	std::string m_parent_name = "";

	// Animation and dragging states
	AnimationState m_anim;
	bool m_is_held = false;
	bool m_is_dragging = false;
	v2s32 m_drag_start_pointer;
	v2s32 m_drag_start_offset;

	// Throttling timers to prevent packet flooding
	u32 m_last_drag_send_time = 0;
	u32 m_last_hold_send_time = 0;
};
