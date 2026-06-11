// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "irr_v3d.h"
#include <vector>
#include <plane3d.h>

/**
 * ViewBounds defines the visible shape of a ViewPort.
 * It is defined by a set of points in 3D space, forming a polygon.
 */
class ViewBounds
{
public:
	ViewBounds() = default;
	ViewBounds(const std::vector<v3f> &points) : m_points(points) {}

	const std::vector<v3f> &getPoints() const { return m_points; }
	void setPoints(const std::vector<v3f> &points) { m_points = points; }

	core::plane3df getPlane() const
	{
		if (m_points.size() < 3)
			return core::plane3df(0, 0, 0, 0, 1, 0);
		return core::plane3df(m_points[0], m_points[1], m_points[2]);
	}

	bool isPointNear(const v3f &point, float threshold) const
	{
		if (m_points.empty())
			return false;

		core::plane3df plane = getPlane();
		float distToPlane = plane.getDistanceTo(point);
		if (std::abs(distToPlane) > threshold)
			return false;

		// Project point to plane
		v3f projected = point - plane.Normal * distToPlane;

		// Check if the projected point is inside the polygon (Winding number or similar)
		// For simplicity and better accuracy than before, check if it's within threshold
		// of ANY point or edge.
		for (size_t i = 0; i < m_points.size(); ++i) {
			const v3f &p1 = m_points[i];
			const v3f &p2 = m_points[(i + 1) % m_points.size()];

			// Distance to segment p1-p2
			v3f v = p2 - p1;
			v3f w = projected - p1;
			float c1 = w.dotProduct(v);
			if (c1 <= 0) {
				if (projected.getDistanceFromSQ(p1) < threshold * threshold) return true;
				continue;
			}
			float c2 = v.dotProduct(v);
			if (c2 <= c1) {
				if (projected.getDistanceFromSQ(p2) < threshold * threshold) return true;
				continue;
			}
			float b = c1 / c2;
			v3f pb = p1 + v * b;
			if (projected.getDistanceFromSQ(pb) < threshold * threshold) return true;
		}

		return false;
	}

private:
	std::vector<v3f> m_points;
};
