// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "serveractiveobject.h"
#include "inventory.h"
#include "inventorymanager.h"
#include "constants.h" // BS
#include "util/serialize.h"
#include "serverenvironment.h"
#include "remoteplayer.h"
#include "server/player_sao.h"

ServerActiveObject::ServerActiveObject(ServerEnvironment *env, v3f pos):
	ActiveObject(0),
	m_env(env),
	m_base_position(pos)
{
}

void ServerActiveObject::setBasePosition(v3f pos)
{
	bool changed = m_base_position != pos;
	m_base_position = pos;
	if (changed && getEnv()) {
		// getEnv() should never be null if the object is in an environment.
		// It may however be null e.g. in tests or database migrations.
		getEnv()->updateObjectPos(getId(), pos);
	}
}

float ServerActiveObject::getMinimumSavedMovement()
{
	return 2.0*BS;
}

ItemStack ServerActiveObject::getWieldedItem(ItemStack *selected, ItemStack *hand) const
{
	*selected = ItemStack();
	if (hand)
		*hand = ItemStack();

	return ItemStack();
}

bool ServerActiveObject::setWieldedItem(const ItemStack &item)
{
	return false;
}

std::string ServerActiveObject::generateUpdateInfantCommand(u16 infant_id, u16 protocol_version)
{
	std::ostringstream os(std::ios::binary);
	// command
	writeU8(os, AO_CMD_SPAWN_INFANT);
	// parameters
	writeU16(os, infant_id);
	writeU8(os, getSendType());
	if (protocol_version < 38) {
		// Clients since 4aa9a66 so no longer need this data
		// Version 38 is the first bump after that commit.
		// See also: ClientEnvironment::addActiveObject
		os << serializeString32(getClientInitializationData(protocol_version));
	}
	return os.str();
}

void ServerActiveObject::dumpAOMessagesToQueue(std::queue<ActiveObjectMessage> &queue)
{
	while (!m_messages_out.empty()) {
		queue.push(std::move(m_messages_out.front()));
		m_messages_out.pop();
	}
}

void ServerActiveObject::markForRemoval()
{
	if (!m_pending_removal) {
		onMarkedForRemoval();
		m_pending_removal = true;
	}
}

void ServerActiveObject::markForDeactivation()
{
	if (!m_pending_deactivation) {
		onMarkedForDeactivation();
		m_pending_deactivation = true;
	}
}

InventoryLocation ServerActiveObject::getInventoryLocation() const
{
	return InventoryLocation();
}

void ServerActiveObject::invalidateEffectiveObservers()
{
	m_effective_observers.reset();
}

using Observers = ServerActiveObject::Observers;

const Observers &ServerActiveObject::getEffectiveObservers()
{
	if (m_effective_observers) // cached
		return *m_effective_observers;

	auto parent = getParent();
	if (parent == nullptr)
		return *(m_effective_observers = m_observers);
	auto parent_observers = parent->getEffectiveObservers();
	if (!parent_observers) // parent is unmanaged
		return *(m_effective_observers = m_observers);
	if (!m_observers) // we are unmanaged
		return *(m_effective_observers = parent_observers);
	// Set intersection between parent_observers and m_observers
	// Avoid .clear() to free the allocated memory.
	m_effective_observers = std::unordered_set<std::string>();
	for (const auto &observer_name : *m_observers) {
		if (parent_observers->count(observer_name) > 0)
			(*m_effective_observers)->insert(observer_name);
	}
	return *m_effective_observers;
}

const Observers& ServerActiveObject::recalculateEffectiveObservers()
{
	// Invalidate final observers for this object and all of its parents.
	for (auto obj = this; obj != nullptr; obj = obj->getParent())
		obj->invalidateEffectiveObservers();
	// getEffectiveObservers will now be forced to recalculate.
	return getEffectiveObservers();
}

bool ServerActiveObject::isEffectivelyObservedBy(const std::string &player_name)
{
	auto effective_observers = getEffectiveObservers();
	return !effective_observers || effective_observers->count(player_name) > 0;
}

const v3f &ServerActiveObject::getRotation() const {
	static v3f dummy;
	return dummy;
}

void ServerActiveObject::startMoveNode(const v3f &pivot, const std::vector<MoveNodeWaypoint> &legs,
	const std::string &easing, const std::string &loop, bool collide,
	const v3f &platform_min, const v3f &platform_max)
{
	m_move_node_state.pivot = pivot;
	m_move_node_state.legs = legs;
	m_move_node_state.easing = easing;
	m_move_node_state.loop = loop;
	m_move_node_state.collide = collide;
	m_move_node_state.platform_min = platform_min;
	m_move_node_state.platform_max = platform_max;

	float total = 0.0f;
	for (const auto &leg : legs) {
		total += leg.duration;
	}
	m_move_node_state.total_duration = total;
	m_move_node_state.elapsed_time = 0.0f;
	m_move_node_state.pingpong_direction = 1;
	m_move_node_state.current_progress = 0.0f;

	if (!legs.empty()) {
		m_move_node_state.start_rot = legs[0].rot_a;
		m_move_node_state.last_rot = legs[0].rot_a;
	}
	m_move_node_state.last_pos = getBasePosition();
	m_move_node_state.active = true;
	m_move_node_state.paused = false;
}

void ServerActiveObject::stopMoveNode()
{
	m_move_node_state.active = false;
}

static float get_eased_time(float t, const std::string &easing)
{
	if (easing == "smoothstep") {
		return t * t * (3.0f - 2.0f * t);
	} else if (easing == "ease_in") {
		return t * t;
	} else if (easing == "ease_out") {
		return t * (2.0f - t);
	} else {
		return t;
	}
}

static float lerp(float a, float b, float t)
{
	return a + (b - a) * t;
}

static float lerp_angle(float a, float b, float t)
{
	float diff = fmod(b - a + M_PI, M_PI * 2.0f) - M_PI;
	if (diff < -M_PI) diff += M_PI * 2.0f;
	return a + diff * t;
}

void ServerActiveObject::stepMoveNode(float dtime)
{
	if (!m_move_node_state.active || m_move_node_state.paused)
		return;

	float next_time = m_move_node_state.elapsed_time + dtime * m_move_node_state.pingpong_direction;
	bool finished = false;

	if (m_move_node_state.pingpong_direction == 1) {
		if (next_time >= m_move_node_state.total_duration) {
			if (m_move_node_state.loop == "pingpong") {
				next_time = m_move_node_state.total_duration;
				m_move_node_state.pingpong_direction = -1;
			} else if (m_move_node_state.loop == "true") {
				next_time = fmod(next_time, m_move_node_state.total_duration);
			} else {
				next_time = m_move_node_state.total_duration;
				finished = true;
			}
		}
	} else {
		if (next_time <= 0.0f) {
			next_time = 0.0f;
			m_move_node_state.pingpong_direction = 1;
		}
	}

	m_move_node_state.elapsed_time = next_time;

	// Find active leg
	const MoveNodeWaypoint *leg = nullptr;
	for (const auto &l : m_move_node_state.legs) {
		if (next_time >= l.start_time && next_time <= l.end_time) {
			leg = &l;
			break;
		}
	}
	if (!leg && !m_move_node_state.legs.empty()) {
		if (next_time < 0.0f) {
			leg = &m_move_node_state.legs.front();
		} else {
			leg = &m_move_node_state.legs.back();
		}
	}

	if (!leg) return;

	float factor = 0.0f;
	if (leg->duration > 0.0001f) {
		factor = (next_time - leg->start_time) / leg->duration;
	} else {
		factor = 1.0f;
	}
	if (factor < 0.0f) factor = 0.0f;
	else if (factor > 1.0f) factor = 1.0f;

	m_move_node_state.current_progress = factor;

	float eased_factor = get_eased_time(factor, m_move_node_state.easing);

	v3f prev_pos = getBasePosition();
	v3f current_pos(
		lerp(leg->pos_a.X, leg->pos_b.X, eased_factor),
		lerp(leg->pos_a.Y, leg->pos_b.Y, eased_factor),
		lerp(leg->pos_a.Z, leg->pos_b.Z, eased_factor)
	);

	v3f current_rot(
		lerp_angle(leg->rot_a.X, leg->rot_b.X, eased_factor),
		lerp_angle(leg->rot_a.Y, leg->rot_b.Y, eased_factor),
		lerp_angle(leg->rot_a.Z, leg->rot_b.Z, eased_factor)
	);

	setBasePosition(current_pos);
	setRotation(current_rot);

	v3f delta = current_pos - prev_pos;

	// 100% smooth player and entity rider support in C++
	if (m_move_node_state.collide && delta.getLength() > 0.0001f && m_env) {
		const std::vector<RemotePlayer *> &players = m_env->getPlayers();
		for (RemotePlayer *player : players) {
			PlayerSAO *sao = player->getPlayerSAO();
			if (!sao) continue;

			v3f ppos = sao->getBasePosition();
			v3f box_min = m_move_node_state.platform_min;
			v3f box_max = m_move_node_state.platform_max;

			float current_min_x = box_min.X + (current_pos.X - m_move_node_state.pivot.X);
			float current_max_x = box_max.X + (current_pos.X - m_move_node_state.pivot.X);
			float current_min_z = box_min.Z + (current_pos.Z - m_move_node_state.pivot.Z);
			float current_max_z = box_max.Z + (current_pos.Z - m_move_node_state.pivot.Z);
			float platform_y = current_pos.Y;

			if (ppos.X >= current_min_x && ppos.X <= current_max_x &&
				ppos.Z >= current_min_z && ppos.Z <= current_max_z &&
				ppos.Y >= platform_y + 0.4f * BS && ppos.Y <= platform_y + 1.2f * BS) {

				sao->setBasePosition(ppos + delta);
			}
		}
	}

	if (finished) {
		m_move_node_state.active = false;
	}
}
