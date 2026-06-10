// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "client/visuals/visual_scene.h"
#include "skyparams.h"
#include <ISceneManager.h>
#include <IrrlichtDevice.h>

VisualScene::VisualScene(IrrlichtDevice *device) :
	m_device(device)
{
	m_smgr = m_device->getSceneManager()->createNewSceneManager();
	m_sky_params = SkyboxDefaults::getSkyDefaults();
}

VisualScene::~VisualScene()
{
	if (m_smgr) {
		m_smgr->clear();
		m_smgr->drop();
	}
}

void VisualScene::activate()
{
	m_active = true;
}

void VisualScene::deactivate()
{
	m_active = false;
}
