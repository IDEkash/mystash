// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "engine/physics_system.h"
#include "engine/registry.h"
#include "settings.h"

PhysicsSystem* PhysicsSystem::m_instance = nullptr;

void PhysicsSystem::Initialize() {
	m_instance = new PhysicsSystem();
	m_instance->sync_from_settings();
	EngineRegistry::expose_raw("physics", m_instance);
}

void PhysicsSystem::sync_from_settings() {
	gravity = g_settings->getFloat("movement_gravity");
	air_resistance = 0.01f; // Default placeholder
	terminal_velocity = 50.0f;
}

void PhysicsSystem::sync_to_settings() {
	g_settings->setFloat("movement_gravity", gravity);
}
