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

void TestDrawPoint::testHitDetection()
{
	// Test hit detection by calling the actual production GUIDrawPoint static helper
	std::vector<v2s32> poly = { v2s32(0, 0), v2s32(100, 0), v2s32(100, 100), v2s32(0, 100) };

	// Center should be inside
	UASSERT(GUIDrawPoint::isPointInsidePolygon(v2s32(50, 50), poly));
	// Points far away should be outside
	UASSERT(!GUIDrawPoint::isPointInsidePolygon(v2s32(150, 50), poly));
	UASSERT(!GUIDrawPoint::isPointInsidePolygon(v2s32(50, -10), poly));
	UASSERT(!GUIDrawPoint::isPointInsidePolygon(v2s32(-10, 50), poly));
	UASSERT(!GUIDrawPoint::isPointInsidePolygon(v2s32(50, 110), poly));
}

void TestDrawPoint::testRoundedCornerClamping()
{
	// Test corner rounding logic and verify radius clamping via actual production function
	std::vector<v2s32> orig_points = { v2s32(0, 0), v2s32(10, 0), v2s32(10, 10), v2s32(0, 10) };
	float radius = 20.0f; // Exceeds edge lengths, should be clamped inside calculateRoundedPoints

	std::vector<v2s32> m_rounded_points = GUIDrawPoint::calculateRoundedPoints(orig_points, radius);

	// With radius clamped to 5.0f (half of edge length 10.0f) across 4 corners,
	// each corner produces 9 points from subdivision.
	UASSERT(m_rounded_points.size() == 36);
}
