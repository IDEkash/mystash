// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "client/visuals/visuals_service.h"
#include "log.h"

VisualsService::VisualsService()
{
}

VisualsService::~VisualsService()
{
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

void VisualsService::evaluateVisibility(const v3f &camera_pos)
{
	m_visible_scenes.clear();

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

		if (is_visible && scene->isVisible()) {
			m_visible_scenes.push_back(scene);
		}
	}
}
