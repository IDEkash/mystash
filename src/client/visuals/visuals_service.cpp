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

void VisualsService::registerScene(const std::string &id, std::unique_ptr<VisualScene> scene)
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

VisualScene *VisualsService::getScene(const std::string &id) const
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

void VisualsService::addRegionToScene(const std::string &scene_id, std::unique_ptr<VisualRegion> region)
{
	m_scene_regions[scene_id].push_back(std::move(region));
}

const std::vector<std::unique_ptr<VisualRegion>> &VisualsService::getRegionsForScene(const std::string &scene_id) const
{
	static const std::vector<std::unique_ptr<VisualRegion>> empty;
	auto it = m_scene_regions.find(scene_id);
	if (it != m_scene_regions.end())
		return it->second;
	return empty;
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

	// 1. Evaluate Scenes
	for (auto &pair : m_scenes) {
		const std::string &id = pair.first;
		VisualScene *scene = pair.second.get();

		bool is_visible = false;

		auto it = m_scene_regions.find(id);
		if (it != m_scene_regions.end()) {
			for (const auto &region : it->second) {
				if (region->contains(camera_pos)) {
					is_visible = true;
					break;
				}
			}
		} else {
			// Scenes without regions are visible by default if they are active
			is_visible = (m_active_scene == scene) || (m_active_scene == nullptr && scene->isVisible());
		}

		if (is_visible && scene->isVisible() && is_layer_visible(scene->getPerspectiveLayer(), camera_mode)) {
			m_visible_scenes.push_back(scene);
		}
	}

	// 2. Evaluate registered nodes
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
