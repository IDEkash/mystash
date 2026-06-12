// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "client/visuals/visuals_service.h"
#include "client/camera.h"
#include "client/client.h"
#include "network/networkpacket.h"
#include "log.h"
#include <ISceneNode.h>

VisualsService::VisualsService(Client *client) : m_client(client)
{
}

VisualsService::~VisualsService()
{
	for (auto &pair : m_registered_nodes) {
		pair.first->drop();
	}
}

void VisualsService::registerScene(u32 id, std::unique_ptr<ViewScene> scene)
{
	if (m_scenes.find(id) != m_scenes.end()) {
		warningstream << "VisualsService: Scene already registered: " << id << std::endl;
		return;
	}
	m_scenes[id] = std::move(scene);
}

void VisualsService::unregisterScene(u32 id)
{
	auto it = m_scenes.find(id);
	if (it != m_scenes.end()) {
		if (m_active_scene == it->second.get())
			m_active_scene = nullptr;

		// Find and remove all zones associated with this scene
		std::vector<u32> zones_to_remove;
		for (auto &pair : m_zone_to_scene) {
			if (pair.second == id)
				zones_to_remove.push_back(pair.first);
		}
		for (u32 zid : zones_to_remove)
			removeRegion(zid);

		m_scenes.erase(it);
		m_scene_regions.erase(id);
	}
}

ViewScene *VisualsService::getScene(u32 id) const
{
	auto it = m_scenes.find(id);
	if (it != m_scenes.end())
		return it->second.get();
	return nullptr;
}

void VisualsService::setActiveScene(u32 id)
{
	if (id == 0)
		m_active_scene = nullptr;
	else
		m_active_scene = getScene(id);
}

void VisualsService::addRegionToScene(u32 id, u32 scene_id, std::unique_ptr<ViewZone> region)
{
	ViewZone *ptr = region.get();
	m_zones[id] = std::move(region);
	m_zone_to_scene[id] = scene_id;
	m_scene_regions[scene_id].push_back(ptr);
}

void VisualsService::removeRegion(u32 id)
{
	auto it = m_zones.find(id);
	if (it != m_zones.end()) {
		u32 scene_id = m_zone_to_scene[id];
		auto &vec = m_scene_regions[scene_id];
		vec.erase(std::remove(vec.begin(), vec.end(), it->second.get()), vec.end());
		m_zones.erase(it);
		m_zone_to_scene.erase(id);
	}
}

const std::vector<ViewZone*> &VisualsService::getRegionsForScene(u32 scene_id) const
{
	static const std::vector<ViewZone*> empty;
	auto it = m_scene_regions.find(scene_id);
	if (it != m_scene_regions.end())
		return it->second;
	return empty;
}

void VisualsService::addViewPort(u32 id, std::unique_ptr<ViewPort> viewport)
{
	m_viewports[id] = std::move(viewport);
}

void VisualsService::removeViewPort(u32 id)
{
	m_viewports.erase(id);
}

const std::vector<ViewPort*> &VisualsService::getViewPorts() const
{
	static std::vector<ViewPort*> list;
	list.clear();
	for (auto &pair : m_viewports)
		list.push_back(pair.second.get());
	return list;
}

void VisualsService::registerNode(scene::ISceneNode *node, const PerspectiveRule &rule, const std::string &tag)
{
	if (m_registered_nodes.find(node) == m_registered_nodes.end()) {
		node->grab();
	}
	m_registered_nodes[node] = {rule, tag};
}

void VisualsService::unregisterNode(scene::ISceneNode *node)
{
	auto it = m_registered_nodes.find(node);
	if (it != m_registered_nodes.end()) {
		node->drop();
		m_registered_nodes.erase(it);
	}
}

void VisualsService::suppressTag(const std::string &tag)
{
	m_active_suppressions.insert(tag);
}

void VisualsService::unsuppressTag(const std::string &tag)
{
	m_active_suppressions.erase(tag);
}

static bool is_layer_visible(PerspectiveLayer layer, CameraMode camera_mode)
{
	switch (layer) {
	case PerspectiveLayer::FirstPerson:
		return camera_mode == CAMERA_MODE_FIRST;
	case PerspectiveLayer::ThirdPerson:
		return camera_mode == CAMERA_MODE_THIRD || camera_mode == CAMERA_MODE_THIRD_FRONT;
	case PerspectiveLayer::Both:
		return true;
	case PerspectiveLayer::Hidden:
		return false;
	}
	return true;
}

void VisualsService::evaluateVisibility(const v3f &camera_pos, CameraMode camera_mode)
{
	m_visible_scenes.clear();
	m_active_viewports.clear();

	std::set<ViewScene*> scenes_to_render;
	std::set<u32> new_inside_zones;
	std::set<u32> new_near_viewports;

	// Evaluate all zones
	for (auto &pair : m_zones) {
		if (pair.second->contains(camera_pos)) {
			new_inside_zones.insert(pair.first);
		}
	}

	// Evaluate all viewports
	for (auto &pair : m_viewports) {
		if (pair.second->getBounds().isPointNear(camera_pos, 2.0f)) {
			new_near_viewports.insert(pair.first);
		}
	}

	auto sendEvent = [&](VisualEventType type, u32 id) {
		NetworkPacket pkt(TOSERVER_VISUAL_EVENT, 5);
		pkt << (u8)type << id;
		m_client->Send(&pkt);
	};

	// Detect zone entries/exits
	for (u32 id : new_inside_zones) {
		if (m_inside_zones.find(id) == m_inside_zones.end())
			sendEvent(VisualEventType::PlayerEnteredZone, id);
	}
	for (u32 id : m_inside_zones) {
		if (new_inside_zones.find(id) == new_inside_zones.end())
			sendEvent(VisualEventType::PlayerLeftZone, id);
	}
	m_inside_zones = std::move(new_inside_zones);

	// Detect viewport proximity changes
	for (u32 id : new_near_viewports) {
		if (m_near_viewports.find(id) == m_near_viewports.end())
			sendEvent(VisualEventType::PlayerEnteredViewPort, id);
	}
	for (u32 id : m_near_viewports) {
		if (new_near_viewports.find(id) == new_near_viewports.end())
			sendEvent(VisualEventType::PlayerLeftViewPort, id);
	}
	m_near_viewports = std::move(new_near_viewports);

	// 1. Evaluate ViewPorts for rendering
	for (auto &pair : m_viewports) {
		u32 vp_id = pair.first;
		ViewPort *vp = pair.second.get();
		ViewScene *scene = getScene(vp->getSceneId());
		if (!scene)
			continue;

		bool in_zone = false;
		auto it = m_scene_regions.find(vp->getSceneId());
		if (it != m_scene_regions.end()) {
			for (const auto &zone : it->second) {
				// We don't have zone IDs in m_scene_regions vector, so we check camera_pos directly
				if (zone->contains(camera_pos)) {
					in_zone = true;
					break;
				}
			}
		}

		bool vp_visible = scene->isVisible();
		bool activate_scene = false;
		bool render_viewport = false;

		switch (vp->getMode()) {
		case ViewMode::ZoneOnly:
			if (in_zone) activate_scene = true;
			break;
		case ViewMode::WindowTrigger:
			if (m_near_viewports.count(vp_id)) activate_scene = true;
			break;
		case ViewMode::WindowAndZone:
			render_viewport = vp_visible;
			if (in_zone) activate_scene = true;
			break;
		case ViewMode::WindowOnly:
			render_viewport = vp_visible;
			break;
		}

		if (activate_scene) {
			scenes_to_render.insert(scene);
		}

		if (render_viewport || vp->getMode() == ViewMode::WindowTrigger) {
			m_active_viewports.push_back({vp, scene});
		}
	}

	// 2. Evaluate Scenes (Global/Active scene)
	u32 best_scene_id = 0;
	for (auto &pair : m_scenes) {
		ViewScene *scene = pair.second.get();
		u32 scene_id = pair.first;
		if (scenes_to_render.count(scene)) {
			if (best_scene_id == 0) best_scene_id = scene_id;
			continue;
		}

		bool is_visible = false;
		auto it = m_scene_regions.find(scene_id);
		if (it == m_scene_regions.end()) {
			is_visible = (m_active_scene == scene) || (m_active_scene == nullptr && scene->isVisible());
		} else {
			for (const auto &zone : it->second) {
				if (zone->contains(camera_pos)) {
					is_visible = true;
					break;
				}
			}
		}

		if (is_visible && scene->isVisible()) {
			scenes_to_render.insert(scene);
			if (best_scene_id == 0) best_scene_id = scene_id;
		}
	}

	// Scene activation events
	if (best_scene_id != m_active_scene_id) {
		if (m_active_scene_id != 0)
			sendEvent(VisualEventType::SceneDeactivated, m_active_scene_id);
		if (best_scene_id != 0)
			sendEvent(VisualEventType::SceneActivated, best_scene_id);
		m_active_scene_id = best_scene_id;
	}

	for (ViewScene *scene : scenes_to_render) {
		if (is_layer_visible(scene->getPerspectiveLayer(), camera_mode)) {
			m_visible_scenes.push_back(scene);
		}
	}

	// 3. Evaluate registered nodes
	std::set<std::string> current_suppressions = m_active_suppressions;

	// First pass: collect automatic suppressions from visible nodes
	for (auto &pair : m_registered_nodes) {
		const RegisteredNode &reg = pair.second;

		if (!is_layer_visible(reg.rule.layer, camera_mode))
			continue;

		bool suppressed = false;
		for (const auto &suppressor : reg.rule.suppressed_by) {
			if (m_active_suppressions.count(suppressor)) {
				suppressed = true;
				break;
			}
		}
		if (!suppressed && !reg.tag.empty() && m_active_suppressions.count(reg.tag))
			suppressed = true;

		if (!suppressed) {
			for (const auto &s : reg.rule.suppresses) {
				current_suppressions.insert(s);
			}
		}
	}

	// Second pass: apply all suppressions
	for (auto &pair : m_registered_nodes) {
		scene::ISceneNode *node = pair.first;
		const RegisteredNode &reg = pair.second;

		bool visible = is_layer_visible(reg.rule.layer, camera_mode);

		if (visible) {
			for (const auto &suppressor : reg.rule.suppressed_by) {
				if (current_suppressions.count(suppressor)) {
					visible = false;
					break;
				}
			}
		}

		if (visible && !reg.tag.empty()) {
			if (current_suppressions.count(reg.tag)) {
				visible = false;
			}
		}

		node->setVisible(visible);
	}
}
