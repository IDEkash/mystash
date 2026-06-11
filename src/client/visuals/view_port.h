// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>
#include "view_bounds.h"

enum class ViewMode
{
	ZoneOnly,
	WindowTrigger,
	WindowAndZone,
	WindowOnly
};

class ViewPort
{
public:
	ViewPort(const std::string &scene_id, ViewMode mode, const ViewBounds &bounds) :
		m_scene_id(scene_id), m_mode(mode), m_bounds(bounds) {}

	const std::string &getSceneId() const { return m_scene_id; }
	ViewMode getMode() const { return m_mode; }
	const ViewBounds &getBounds() const { return m_bounds; }

	void setMode(ViewMode mode) { m_mode = mode; }
	void setBounds(const ViewBounds &bounds) { m_bounds = bounds; }

private:
	std::string m_scene_id;
	ViewMode m_mode;
	ViewBounds m_bounds;
};
