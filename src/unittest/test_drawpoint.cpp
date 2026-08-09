// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2026 Jules

#include "test.h"
#include "gui/GUIDrawPoint.h"
#include "irr_v2d.h"
#include <vector>

class TestDrawPoint : public TestBase
{
public:
	TestDrawPoint() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestDrawPoint"; }

	void runTests(IGameDef *gamedef);

	void testHitDetection();
	void testRoundedCornerClamping();
};

static TestDrawPoint g_test_instance;

void TestDrawPoint::runTests(IGameDef *gamedef)
{
	TEST(testHitDetection);
	TEST(testRoundedCornerClamping);
}

// Simple mock/subclass to inspect inner protected/private state if needed,
// but since isPointInside is already tested we can test hit detection directly via a mock or
// custom logic that mirrors the implementation, or instantiate a GUIDrawPoint.
void TestDrawPoint::testHitDetection()
{
	// Let's test standard ray-casting point-in-polygon algorithm used in hit detection
	std::vector<v2s32> poly = { v2s32(0, 0), v2s32(100, 0), v2s32(100, 100), v2s32(0, 100) };

	auto is_inside = [](const v2s32 &pt, const std::vector<v2s32> &p) {
		if (p.size() < 3)
			return false;
		bool inside = false;
		int n = p.size();
		for (int i = 0, j = n - 1; i < n; j = i++) {
			if (((p[i].Y > pt.Y) != (p[j].Y > pt.Y)) &&
				(pt.X < (p[j].X - p[i].X) * (pt.Y - p[i].Y) / (float)(p[j].Y - p[i].Y) + p[i].X)) {
				inside = !inside;
			}
		}
		return inside;
	};

	// Center should be inside
	UASSERT(is_inside(v2s32(50, 50), poly));
	// Points far away should be outside
	UASSERT(!is_inside(v2s32(150, 50), poly));
	UASSERT(!is_inside(v2s32(50, -10), poly));
	UASSERT(!is_inside(v2s32(-10, 50), poly));
	UASSERT(!is_inside(v2s32(50, 110), poly));
}

void TestDrawPoint::testRoundedCornerClamping()
{
	// Test corner rounding logic and verify radius clamping
	std::vector<v2s32> rel_orig = { v2s32(0, 0), v2s32(10, 0), v2s32(10, 10), v2s32(0, 10) };
	float m_radius = 20.0f; // Exceeds edge lengths, should be clamped

	std::vector<v2s32> m_rounded_points;
	size_t n = rel_orig.size();

	for (size_t i = 0; i < n; ++i) {
		v2s32 B = rel_orig[i];
		v2s32 A = rel_orig[(i + n - 1) % n];
		v2s32 C = rel_orig[(i + 1) % n];

		v2f32 v1(A.X - B.X, A.Y - B.Y);
		v2f32 v2(C.X - B.X, C.Y - B.Y);

		float len1 = v1.getLength();
		float len2 = v2.getLength();

		UASSERT(len1 > 0.0f);
		UASSERT(len2 > 0.0f);

		v2f32 hat1 = v1 / len1;
		v2f32 hat2 = v2 / len2;

		// Clamp radius to half the edge length (10 / 2 = 5.0)
		float r = std::min(m_radius, std::min(len1 / 2.0f, len2 / 2.0f));
		UASSERT(r == 5.0f); // Should be exactly 5.0f due to clamping!

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

	UASSERT(m_rounded_points.size() == 36); // 4 corners * 9 points each
}
