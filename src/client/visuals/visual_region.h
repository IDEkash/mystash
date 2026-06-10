// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "irr_v3d.h"
#include "irr_aabb3d.h"

/**
 * A region defines where a visual scene may appear.
 * Outside an authorized region, scene content must not render.
 */
class VisualRegion
{
public:
	VisualRegion(const aabb3f &bounds) : m_bounds(bounds) {}

	const aabb3f &getBounds() const { return m_bounds; }
	void setBounds(const aabb3f &bounds) { m_bounds = bounds; }

	bool contains(const v3f &point) const
	{
		return m_bounds.isPointInside(point);
	}

	bool intersects(const aabb3f &other) const
	{
		return m_bounds.intersectsWithBox(other);
	}

private:
	aabb3f m_bounds;
};
