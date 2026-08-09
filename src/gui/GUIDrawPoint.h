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
public:
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

	std::wstring m_text;

	bool m_is_held = false;
};
