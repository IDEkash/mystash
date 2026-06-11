// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "client/visuals/visuals_service.h"
#include "client/camera.h"
#include "log.h"
#include <ISceneNode.h>

VisualsService::VisualsService()
{
}

VisualsService::~VisualsService()
{
	for (auto &pair : m_registered_nodes) {
		pair.first->drop();
	}
}

void VisualsService::registerScene(const std::string &id, std::unique_ptr<ViewScene> scene)
{
	if (m_scenes.find(id) != m_scenes.end()) {
		warningstream << "VisualsService: Scene already registered: " << id << std::endl;
		return;
	}
	m_scenes[id] = std::move(scene);
}

void VisualsService::unregisterScene(const std::string &id)
{
	auto it = m_scenes.find(id);
	if (it != m_scenes.end()) {
		if (m_active_scene == it->second.get())
			m_active_scene = nullptr;
		m_scenes.erase(it);
		m_scene_regions.erase(id);
	}
}

ViewScene *VisualsService::getScene(const std::string &id) const
{
	auto it = m_scenes.find(id);
	if (it != m_scenes.end())
		return it->second.get();
	return nullptr;
}

void VisualsService::setActiveScene(const std::string &id)
{
	m_active_scene = getScene(id);
}

void VisualsService::addRegionToScene(const std::string &scene_id, std::unique_ptr<ViewZone> region)
{
	m_scene_regions[scene_id].push_back(std::move(region));
}

const std::vector<std::unique_ptr<ViewZone>> &VisualsService::getRegionsForScene(const std::string &scene_id) const
{
	static const std::vector<std::unique_ptr<ViewZone>> empty;
	auto it = m_scene_regions.find(scene_id);
	if (it != m_scene_regions.end())
		return it->second;
	return empty;
}

void VisualsService::addViewPort(std::unique_ptr<ViewPort> viewport)
{
	m_viewports.push_back(std::move(viewport));
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

	// 1. Evaluate ViewPorts
	for (const auto &vp : m_viewports) {
		ViewScene *scene = getScene(vp->getSceneId());
		if (!scene)
			continue;

		bool in_zone = false;
		auto it = m_scene_regions.find(vp->getSceneId());
		if (it != m_scene_regions.end()) {
			for (const auto &zone : it->second) {
				if (zone->contains(camera_pos)) {
					in_zone = true;
					break;
				}
			}
		}

		// TODO: Implement proper viewport visibility check (frustum culling, etc.)
		// For now we assume they might be visible if the scene is visible.
		bool vp_visible = scene->isVisible();

		bool activate_scene = false;
		bool render_viewport = false;

		switch (vp->getMode()) {
		case ViewMode::ZoneOnly:
			if (in_zone)
				activate_scene = true;
			break;
		case ViewMode::WindowTrigger:
			// "Entering the viewport activates the scene"
			if (vp->getBounds().isPointNear(camera_pos, 2.0f))
				activate_scene = true;
			break;
		case ViewMode::WindowAndZone:
			render_viewport = vp_visible;
			if (in_zone)
				activate_scene = true;
			break;
		case ViewMode::WindowOnly:
			render_viewport = vp_visible;
			break;
		}

		if (activate_scene) {
			scenes_to_render.insert(scene);
		}

		if (render_viewport || vp->getMode() == ViewMode::WindowTrigger) {
			m_active_viewports.push_back({vp.get(), scene});
		}
	}

	// 2. Evaluate Scenes (Global/Active scene)
	for (auto &pair : m_scenes) {
		ViewScene *scene = pair.second.get();
		if (scenes_to_render.count(scene))
			continue;

		bool is_visible = false;
		auto it = m_scene_regions.find(pair.first);
		if (it == m_scene_regions.end()) {
			// Scenes without regions are visible by default if they are active
			is_visible = (m_active_scene == scene) || (m_active_scene == nullptr && scene->isVisible());
		} else {
			// If it has regions but no viewport was defined for it,
			// we can still activate it if we are in a region.
			for (const auto &zone : it->second) {
				if (zone->contains(camera_pos)) {
					is_visible = true;
					break;
				}
			}
		}

		if (is_visible && scene->isVisible()) {
			scenes_to_render.insert(scene);
		}
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
