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

enum class VisualEventType
{
	PlayerEnteredZone = 1,
	PlayerLeftZone = 2,
	PlayerEnteredViewPort = 3,
	PlayerLeftViewPort = 4,
	SceneActivated = 5,
	SceneDeactivated = 6
};

class ViewPort
{
public:
	ViewPort(u32 scene_id, ViewMode mode, const ViewBounds &bounds) :
		m_scene_id(scene_id), m_mode(mode), m_bounds(bounds) {}

	u32 getSceneId() const { return m_scene_id; }
	ViewMode getMode() const { return m_mode; }
	const ViewBounds &getBounds() const { return m_bounds; }

	void setMode(ViewMode mode) { m_mode = mode; }
	void setBounds(const ViewBounds &bounds) { m_bounds = bounds; }

private:
	u32 m_scene_id;
	ViewMode m_mode;
	ViewBounds m_bounds;
};
