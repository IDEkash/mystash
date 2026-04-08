// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>
#include <vector>
#include "engine/meta_macros.h"

class PlayerSAO;

class DimensionManager {
	REFLECT_CLASS()
public:
	static void Initialize();

	void send_player(PlayerSAO *player, const std::string &dimension_id);

	REFLECT_FIELD() std::string m_active_dimension = "overworld";
	REFLECT_FIELD() std::vector<std::string> m_dimension_list;

private:
	static DimensionManager *m_instance;
};
