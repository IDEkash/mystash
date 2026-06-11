// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "client/visuals/view_scene.h"
#include "skyparams.h"
#include <ISceneManager.h>
#include <IrrlichtDevice.h>

ViewScene::ViewScene(IrrlichtDevice *device) :
	m_device(device),
	m_perspective_layer(PerspectiveLayer::Both)
{
	m_smgr = m_device->getSceneManager()->createNewSceneManager();
	m_sky_params = SkyboxDefaults::getSkyDefaults();
}

ViewScene::~ViewScene()
{
	if (m_smgr) {
		m_smgr->clear();
		m_smgr->drop();
	}
}

void ViewScene::activate()
{
	m_active = true;
}

void ViewScene::deactivate()
{
	m_active = false;
}
