// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "irrlichttypes_bloated.h"
#include "skyparams.h"
#include "fogparams.h"
#include <vector>

enum class PerspectiveLayer;

class IrrlichtDevice;
namespace scene {
	class ISceneManager;
}

class VisualScene
{
public:
	VisualScene(IrrlichtDevice *device);
	virtual ~VisualScene();

	// Deleted copy constructor and assignment operator to ensure explicit ownership
	VisualScene(const VisualScene &) = delete;
	VisualScene &operator=(const VisualScene &) = delete;

	scene::ISceneManager *getSceneManager() const { return m_smgr; }

	// Sky settings
	void setSkyParams(const SkyboxParams &params) { m_sky_params = params; }
	const SkyboxParams &getSkyParams() const { return m_sky_params; }

	// Fog settings
	void setFogParams(const FogParams &params) { m_fog_params = params; }
	const FogParams &getFogParams() const { return m_fog_params; }

	// Environmental effects
	// TODO: Add more environmental settings (lighting, etc.)

	// Visibility
	void setVisible(bool visible) { m_visible = visible; }
	bool isVisible() const { return m_visible; }

	void setPerspectiveLayer(PerspectiveLayer layer) { m_perspective_layer = layer; }
	PerspectiveLayer getPerspectiveLayer() const { return m_perspective_layer; }

	// Lifecycle
	virtual void activate();
	virtual void deactivate();

protected:
	IrrlichtDevice *m_device;
	scene::ISceneManager *m_smgr;

	SkyboxParams m_sky_params;
	FogParams m_fog_params;

	PerspectiveLayer m_perspective_layer;

	bool m_visible = true;
	bool m_active = false;
};
