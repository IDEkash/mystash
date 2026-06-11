// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "view_scene.h"
#include "view_zone.h"
#include "view_port.h"
#include "visual_perspective.h"
#include "client/camera.h"
#include "player.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <set>

namespace scene {
	class ISceneNode;
}

class VisualsService
{
public:
	VisualsService();
	~VisualsService();

	// Scene management
	void registerScene(const std::string &id, std::unique_ptr<ViewScene> scene);
	void unregisterScene(const std::string &id);
	ViewScene *getScene(const std::string &id) const;

	// Visibility evaluation
	void setActiveScene(const std::string &id);
	ViewScene *getActiveScene() const { return m_active_scene; }

	// Region management
	void addRegionToScene(const std::string &scene_id, std::unique_ptr<ViewZone> region);
	const std::vector<std::unique_ptr<ViewZone>> &getRegionsForScene(const std::string &scene_id) const;

	// ViewPort management
	void addViewPort(std::unique_ptr<ViewPort> viewport);
	const std::vector<std::unique_ptr<ViewPort>> &getViewPorts() const { return m_viewports; }

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

	const std::vector<ViewScene*> &getVisibleScenes() const { return m_visible_scenes; }

	struct ActiveViewPort {
		ViewPort *viewport;
		ViewScene *scene;
	};
	const std::vector<ActiveViewPort> &getActiveViewPorts() const { return m_active_viewports; }

private:
	std::unordered_map<std::string, std::unique_ptr<ViewScene>> m_scenes;
	std::unordered_map<std::string, std::vector<std::unique_ptr<ViewZone>>> m_scene_regions;
	std::vector<std::unique_ptr<ViewPort>> m_viewports;

	struct RegisteredNode {
		PerspectiveRule rule;
		std::string tag;
	};
	std::unordered_map<scene::ISceneNode*, RegisteredNode> m_registered_nodes;
	std::set<std::string> m_active_suppressions;

	ViewScene *m_active_scene = nullptr;
	std::vector<ViewScene*> m_visible_scenes;
	std::vector<ActiveViewPort> m_active_viewports;
};
