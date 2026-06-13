// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "irrlichttypes.h"
#include <set>
#include <string>

enum class PerspectiveLayer : u8
{
	FirstPerson = 0,
	ThirdPerson = 1,
	Both = 2,
	Hidden = 3
};

struct PerspectiveRule
{
	PerspectiveLayer layer = PerspectiveLayer::Both;
	std::set<std::string> suppressed_by;
	std::set<std::string> suppresses;
};
