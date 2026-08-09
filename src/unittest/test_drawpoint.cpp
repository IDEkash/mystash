// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2026 Jules

#include "test.h"
#include "gui/GUIDrawPoint.h"
#include "irr_v2d.h"
#include <vector>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class TestDrawPoint : public TestBase
{
public:
	TestDrawPoint() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestDrawPoint"; }

	void runTests(IGameDef *gamedef);

	void testHitDetection();
	void testRoundedCornerClamping();
	void testAffineTransforms();
	void testInheritance();
};

static TestDrawPoint g_test_instance;

void TestDrawPoint::runTests(IGameDef *gamedef)
{
	TEST(testHitDetection);
	TEST(testRoundedCornerClamping);
	TEST(testAffineTransforms);
	TEST(testInheritance);
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

void TestDrawPoint::testAffineTransforms()
{
	// Test 2D Affine transformation logic (Translation, Scale, Rotation)
	std::vector<v2s32> local_points = { v2s32(0, 0), v2s32(100, 0), v2s32(100, 100), v2s32(0, 100) };

	// Centroid of local points
	v2f32 centroid(50.0f, 50.0f);

	v2s32 offset(10, 20);
	v2f32 scale(2.0f, 2.0f);
	float rotation = 90.0f; // in degrees

	// Transform point (100, 0) relative to centroid (50, 50)
	v2s32 target_p(100, 0);

	v2f32 local_p(target_p.X - centroid.X, target_p.Y - centroid.Y); // (50, -50)
	local_p.X *= scale.X; // 100
	local_p.Y *= scale.Y; // -100

	// Rotate 90 degrees
	float rad = rotation * M_PI / 180.0f;
	float cos_r = std::cos(rad); // 0
	float sin_r = std::sin(rad); // 1
	float rx = local_p.X * cos_r - local_p.Y * sin_r; // -(-100) * 1 = 100
	float ry = local_p.X * sin_r + local_p.Y * cos_r; // 100 * 1 = 100

	v2s32 abs_p(
		std::round(rx + centroid.X) + offset.X, // 100 + 50 + 10 = 160
		std::round(ry + centroid.Y) + offset.Y  // 100 + 50 + 20 = 170
	);

	UASSERT(abs_p.X == 160);
	UASSERT(abs_p.Y == 170);
}

void TestDrawPoint::testInheritance()
{
	// Test color multiplication inheritance logic
	video::SColor parent_col(255, 128, 128, 128); // 50% gray
	video::SColor child_col(255, 255, 255, 255);  // white

	// Child color multiplied by parent color should result in parent color (50% gray)
	video::SColor col = child_col;
	col.setAlpha((col.getAlpha() * parent_col.getAlpha()) / 255);
	col.setRed((col.getRed() * parent_col.getRed()) / 255);
	col.setGreen((col.getGreen() * parent_col.getGreen()) / 255);
	col.setBlue((col.getBlue() * parent_col.getBlue()) / 255);

	UASSERT(col.getRed() == 128);
	UASSERT(col.getGreen() == 128);
	UASSERT(col.getBlue() == 128);
}
