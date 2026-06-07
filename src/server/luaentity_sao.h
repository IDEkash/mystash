// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2013-2020 Minetest core developers & community

#pragma once

#include "unit_sao.h"
#include "util/guid.h"

class LuaEntitySAO : public UnitSAO
{
public:
	LuaEntitySAO() = delete;
	// Used by the environment to load SAO
	LuaEntitySAO(ServerEnvironment *env, v3f pos, const std::string &data);
	// Used by the Lua API
	LuaEntitySAO(ServerEnvironment *env, v3f pos, const std::string &name,
			const std::string &state);
	~LuaEntitySAO();

	ActiveObjectType getType() const override { return ACTIVEOBJECT_TYPE_LUAENTITY; }
	ActiveObjectType getSendType() const override { return ACTIVEOBJECT_TYPE_GENERIC; }
	void addedToEnvironment(u32 dtime_s) override;
	void step(float dtime, bool send_recommended) override;
	std::string getClientInitializationData(u16 protocol_version, const std::string &viewer = "") override;

	bool isStaticAllowed() const override { return m_prop.static_save; }
	bool shouldUnload() const override { return true; }
	void getStaticData(std::string *result) const override;

	u32 punch(v3f dir, const ToolCapabilities &toolcap,
			ServerActiveObject *puncher = nullptr,
			float time_from_last_punch = 1000000.0f,
			u16 initial_wear = 0) override;

	void rightClick(ServerActiveObject *clicker) override;

	void setPos(const v3f &pos) override;
	void moveTo(v3f pos, bool continuous) override;
	float getMinimumSavedMovement() override;

	std::string getDescription() override;

	void setHP(s32 hp, const PlayerHPChangeReason &reason) override;
	u16 getHP() const override;
	std::string getGUID() const override;

	/* LuaEntitySAO-specific */
	void setVelocity(v3f velocity);
	void addVelocity(v3f velocity) { m_velocity += velocity; }
	v3f getVelocity();
	void setAcceleration(v3f acceleration);
	v3f getAcceleration();

	void setTextureMod(const std::string &mod);
	std::string getTextureMod() const;
	void setSprite(v2s16 p, int num_frames, float framelength,
			bool select_horiz_by_yawpitch);
	std::string getName();
	bool getCollisionBox(aabb3f *toset) const override;
	bool getSelectionBox(aabb3f *toset) const override;
	bool collideWithObjects() const override;

protected:
	void dispatchScriptDeactivate(bool removal);
	void onMarkedForDeactivation() override {
		UnitSAO::onMarkedForDeactivation();
		dispatchScriptDeactivate(false);
	}
	void onMarkedForRemoval() override {
		UnitSAO::onMarkedForRemoval();
		dispatchScriptDeactivate(true);
	}

private:
	std::string getPropertyPacket();
	void sendPosition(bool do_interpolate, bool is_movement_end);
	std::string generateSetTextureModCommand() const;
	static std::string generateSetSpriteCommand(v2s16 p, u16 num_frames,
			f32 framelength, bool select_horiz_by_yawpitch);

	std::string m_init_name;
	std::string m_init_state;
	bool m_registered = false;

	MyGUID m_guid;

	v3f m_velocity;
	v3f m_acceleration;

	v3f m_last_sent_position;
	v3f m_last_sent_velocity;
	v3f m_last_sent_rotation;
	float m_last_sent_position_timer = 0.0f;
	float m_last_sent_move_precision = 0.0f;

	std::string m_texture_modifier;
	bool m_texture_modifier_sent = false;
};
