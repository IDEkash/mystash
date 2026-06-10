// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "visual_scene.h"
#include "visual_region.h"
#include "player.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <set>

enum class PerspectiveLayer
{
	FirstPerson,
	ThirdPerson,
	Both,
	Hidden
};

struct PerspectiveRule
{
	PerspectiveLayer layer = PerspectiveLayer::Both;
	std::set<std::string> suppressed_by;
	std::set<std::string> suppresses;
};

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

	// Node perspective management
	void registerNode(scene::ISceneNode *node, const PerspectiveRule &rule, const std::string &tag = "");
	void unregisterNode(scene::ISceneNode *node);

	// Suppression management
	void suppressTag(const std::string &tag);
	void unsuppressTag(const std::string &tag);

	/**
	 * Evaluate visibility for all registered scenes and nodes based on current camera position and mode.
	 */
	void evaluateVisibility(const v3f &camera_pos, CameraMode camera_mode);

	const std::vector<VisualScene*> &getVisibleScenes() const { return m_visible_scenes; }

private:
	std::unordered_map<std::string, std::unique_ptr<VisualScene>> m_scenes;
	std::unordered_map<std::string, std::vector<std::unique_ptr<VisualRegion>>> m_scene_regions;

	struct RegisteredNode {
		PerspectiveRule rule;
		std::string tag;
	};
	std::unordered_map<scene::ISceneNode*, RegisteredNode> m_registered_nodes;
	std::set<std::string> m_active_suppressions;

	VisualScene *m_active_scene = nullptr;
	std::vector<VisualScene*> m_visible_scenes;
};
