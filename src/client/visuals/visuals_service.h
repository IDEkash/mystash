// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "visual_scene.h"
#include "visual_region.h"
#include <vector>
#include <memory>
#include <unordered_map>

class VisualsService
{
public:
	VisualsService();
	~VisualsService();

	// Scene management
	void registerScene(const std::string &id, std::unique_ptr<VisualScene> scene);
	void unregisterScene(const std::string &id);
	VisualScene *getScene(const std::string &id) const;

	// Visibility evaluation
	void setActiveScene(const std::string &id);
	VisualScene *getActiveScene() const { return m_active_scene; }

	// Region management
	void addRegionToScene(const std::string &scene_id, std::unique_ptr<VisualRegion> region);
	const std::vector<std::unique_ptr<VisualRegion>> &getRegionsForScene(const std::string &scene_id) const;

	/**
	 * Evaluate visibility for all registered scenes based on current camera position.
	 */
	void evaluateVisibility(const v3f &camera_pos);

	const std::vector<VisualScene*> &getVisibleScenes() const { return m_visible_scenes; }

private:
	std::unordered_map<std::string, std::unique_ptr<VisualScene>> m_scenes;
	std::unordered_map<std::string, std::vector<std::unique_ptr<VisualRegion>>> m_scene_regions;

	VisualScene *m_active_scene = nullptr;
	std::vector<VisualScene*> m_visible_scenes;
};
