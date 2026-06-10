// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <set>
#include <string>

enum class PerspectiveLayer
{
	FirstPerson,
	ThirdPerson,
	Both,
	Hidden
};

struct PerspectiveRule
{
	PerspectiveLayer layer = PerspectiveLayer::Both;
	std::set<std::string> suppressed_by;
	std::set<std::string> suppresses;
};
