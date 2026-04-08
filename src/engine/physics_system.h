// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "engine/meta_macros.h"

// Proxy system for global physics settings
class PhysicsSystem {
	REFLECT_CLASS()
public:
	static void Initialize();

	REFLECT_FIELD() float gravity;
	REFLECT_FIELD() float air_resistance;
	REFLECT_FIELD() float terminal_velocity;

	// Simple logic to sync with g_settings for demonstration
	void sync_to_settings();
	void sync_from_settings();

	static PhysicsSystem* getInstance() { return m_instance; }

private:
	static PhysicsSystem *m_instance;
};
