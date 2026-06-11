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
	ViewBounds(const std::vector<v3f> &points) : m_points(points)
	{
		updateBasis();
	}

	const std::vector<v3f> &getPoints() const { return m_points; }
	void setPoints(const std::vector<v3f> &points)
	{
		m_points = points;
		updateBasis();
	}

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

		// Check if the projected point is inside the polygon using ray casting algorithm.
		auto get2D = [&](const v3f &p) {
			return v2f(p.dotProduct(m_v1), p.dotProduct(m_v2));
		};

		v2f p2d = get2D(projected);
		bool inside = false;
		for (size_t i = 0, j = m_points.size() - 1; i < m_points.size(); j = i++) {
			v2f pi = get2D(m_points[i]);
			v2f pj = get2D(m_points[j]);
			if (((pi.Y > p2d.Y) != (pj.Y > p2d.Y)) &&
				(p2d.X < (pj.X - pi.X) * (p2d.Y - pi.Y) / (pj.Y - pi.Y) + pi.X)) {
				inside = !inside;
			}
		}

		if (inside)
			return true;

		// Also check if it's within threshold of ANY edge (for fuzzy boundaries)
		for (size_t i = 0; i < m_points.size(); ++i) {
			const v3f &p1 = m_points[i];
			const v3f &p2 = m_points[(i + 1) % m_points.size()];

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
	void updateBasis()
	{
		if (m_points.size() < 3) {
			m_v1 = v3f(1, 0, 0);
			m_v2 = v3f(0, 0, 1);
			return;
		}
		core::plane3df plane = getPlane();
		if (std::abs(plane.Normal.X) > 0.1f || std::abs(plane.Normal.Z) > 0.1f)
			m_v1 = v3f(0, 1, 0).crossProduct(plane.Normal).normalize();
		else
			m_v1 = v3f(1, 0, 0).crossProduct(plane.Normal).normalize();
		m_v2 = plane.Normal.crossProduct(m_v1).normalize();
	}

	std::vector<v3f> m_points;
	v3f m_v1;
	v3f m_v2;
};
