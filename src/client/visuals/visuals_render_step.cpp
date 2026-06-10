// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "client/visuals/visuals_render_step.h"
#include "client/client.h"
#include "client/visuals/visuals_service.h"
#include "client/visuals/visual_scene.h"
#include "client/camera.h"
#include <ISceneManager.h>
#include <ICameraSceneNode.h>
#include <IVideoDriver.h>

void VisualsRenderStep::run(PipelineContext &context)
{
	if (m_target)
		m_target->activate(context);

	VisualsService *visuals = context.client->getVisualsService();
	if (!visuals)
		return;

	scene::ISceneManager *main_smgr = context.device->getSceneManager();
	scene::ICameraSceneNode *active_cam = main_smgr->getActiveCamera();
	if (!active_cam)
		return;

	CameraMode camera_mode = CAMERA_MODE_FIRST;
	if (context.client->getCamera()) {
		camera_mode = context.client->getCamera()->getCameraMode();
	}

	// Evaluate visibility before rendering
	visuals->evaluateVisibility(active_cam->getPosition(), camera_mode);

	// Render all visible scenes
	for (VisualScene *scene : visuals->getVisibleScenes()) {
		scene::ISceneManager *smgr = scene->getSceneManager();
		if (!smgr)
			continue;

		// Synchronize camera with main camera
		scene::ICameraSceneNode *scene_cam = smgr->getActiveCamera();
		if (!scene_cam) {
			scene_cam = smgr->addCameraSceneNode();
		}
		scene_cam->setPosition(active_cam->getPosition());
		scene_cam->setTarget(active_cam->getTarget());
		scene_cam->setUpVector(active_cam->getUpVector());
		scene_cam->setProjectionMatrix(active_cam->getProjectionMatrix());

		// Apply fog settings
		const FogParams &fp = scene->getFogParams();
		if (fp.active) {
			context.device->getVideoDriver()->setFog(
				fp.color,
				video::EFT_FOG_LINEAR,
				fp.fog_start,
				fp.fog_end,
				0.01f,
				false,
				true
			);
		}

		smgr->drawAll();
	}
}
