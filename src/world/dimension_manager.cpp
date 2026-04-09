// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "world/dimension_manager.h"
#include "engine/registry.h"
#include "log.h"

DimensionManager* DimensionManager::m_instance = nullptr;

void DimensionManager::Initialize() {
	m_instance = new DimensionManager();
	EngineRegistry::expose_raw("dimensions", m_instance);

	EngineRegistry::expose_method("dimensions", "send_player", &DimensionManager::send_player);
	EngineRegistry::expose_method("dimensions", "register_dimension", &DimensionManager::register_dimension);
	EngineRegistry::expose_property("dimensions", "active_dimension", &DimensionManager::m_active_dimension);
}

void DimensionManager::register_dimension(const std::string &id, const std::string &config) {
	infostream << "DimensionManager: Registering dimension " << id << std::endl;
	m_dimension_list.push_back(id);
}

void DimensionManager::send_player(PlayerSAO *player, const std::string &dimension_id) {
	infostream << "DimensionManager: Sending player to " << dimension_id << std::endl;
	// Logic to change player's environment or coordinate offset would go here
}
