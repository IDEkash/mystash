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

class Client;

class VisualsService
{
public:
	VisualsService(Client *client);
	~VisualsService();

	// Scene management
	void registerScene(u32 id, std::unique_ptr<ViewScene> scene);
	void unregisterScene(u32 id);
	ViewScene *getScene(u32 id) const;

	// Visibility evaluation
	void setActiveScene(u32 id);
	ViewScene *getActiveScene() const { return m_active_scene; }

	// Region management
	void addRegionToScene(u32 id, u32 scene_id, std::unique_ptr<ViewZone> region);
	void removeRegion(u32 id);
	const std::vector<ViewZone*> &getRegionsForScene(u32 scene_id) const;

	// ViewPort management
	void addViewPort(u32 id, std::unique_ptr<ViewPort> viewport);
	void removeViewPort(u32 id);
	const std::vector<ViewPort*> &getViewPorts() const;

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
	Client *m_client;
	std::unordered_map<u32, std::unique_ptr<ViewScene>> m_scenes;
	std::unordered_map<u32, std::unique_ptr<ViewZone>> m_zones;
	std::unordered_map<u32, u32> m_zone_to_scene;
	std::unordered_map<u32, std::vector<ViewZone*>> m_scene_regions;
	std::unordered_map<u32, std::unique_ptr<ViewPort>> m_viewports;

	struct RegisteredNode {
		PerspectiveRule rule;
		std::string tag;
	};
	std::unordered_map<scene::ISceneNode*, RegisteredNode> m_registered_nodes;
	std::set<std::string> m_active_suppressions;

	ViewScene *m_active_scene = nullptr;
	u32 m_active_scene_id = 0;
	std::vector<ViewScene*> m_visible_scenes;
	std::vector<ActiveViewPort> m_active_viewports;

	std::set<u32> m_inside_zones;
	std::set<u32> m_near_viewports;
};
