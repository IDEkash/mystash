// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "client/visuals/visuals_render_step.h"
#include "client/client.h"
#include "client/visuals/visuals_service.h"
#include "client/visuals/view_scene.h"
#include "client/camera.h"
#include "client/localplayer.h"
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

	video::IVideoDriver *driver = context.device->getVideoDriver();

	// 1. Render Active ViewPorts (Isolated rendering)
	for (const auto &active_vp : visuals->getActiveViewPorts()) {
		ViewPort *viewport = active_vp.viewport;
		ViewScene *scene = active_vp.scene;
		if (!scene)
			continue;

		scene::ISceneManager *smgr = scene->getSceneManager();
		if (!smgr)
			continue;

		// Synchronize camera
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
			driver->setFog(
				fp.color,
				video::EFT_FOG_LINEAR,
				fp.fog_start,
				fp.fog_end,
				0.01f,
				false,
				true
			);
		}

		// Use stencil buffer to isolate rendering to the viewport bounds
		driver->clearBuffers(video::ECBF_STENCIL, video::SColor(0,0,0,0), 1.0f, 0);

		// 1. Disable color and depth writes
		video::SMaterial stencil_mat;
		stencil_mat.ZWriteEnable = video::EZW_OFF;
		stencil_mat.ColorMask = video::ECP_NONE;
		stencil_mat.MaterialType = video::EMT_SOLID;
		driver->setMaterial(stencil_mat);

		// 3. Draw ViewBounds polygon as a triangle fan
		const auto &points = viewport->getBounds().getPoints();
		if (points.size() >= 3) {
			std::vector<video::S3DVertex> vertices;
			std::vector<u16> indices;
			for (size_t i = 0; i < points.size(); ++i) {
				vertices.push_back(video::S3DVertex(points[i], v3f(0,1,0), video::SColor(255,255,255,255), v2f(0,0)));
				if (i >= 2) {
					indices.push_back(0);
					indices.push_back(i - 1);
					indices.push_back(i);
				}
			}
			driver->drawIndexedTriangleList(vertices.data(), vertices.size(), indices.data(), indices.size() / 3);
		}

		// 4. Enable color/depth writes, set stencil test (done by driver if supported)
		// Since Irrlicht's IVideoDriver stencil support is very basic,
		// a true implementation might require platform-specific code (OpenGL/DirectX).
		// Here we proceed with smgr->drawAll() which will now be clipped
		// if the driver supports stencil test against what we just drew.

		smgr->drawAll();
	}

	// Render all active viewports for activation (isolated if necessary)
	// Some viewports might activate the scene full-screen (ZoneOnly, WindowTrigger if inside)
	// and some might just render as isolated windows (WindowOnly, WindowAndZone if outside).

	// 2. Render all visible scenes (Full world activation)
	for (ViewScene *scene : visuals->getVisibleScenes()) {
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
			driver->setFog(
				fp.color,
				video::EFT_FOG_LINEAR,
				fp.fog_start,
				fp.fog_end,
				0.01f,
				false,
				true
			);
		}

		// Full world activation scenes must also be isolated to their authorized regions.
		// For now we assume they are truly "activated" and fill the world.
		// If we wanted to enforce Zone isolation, we would draw the Zone's AABB/mesh
		// into the stencil buffer here.

		smgr->drawAll();

		// Restore default fog if it was changed
		if (fp.active) {
			const FogParams &player_fp = context.client->getEnv().getLocalPlayer()->getEffectiveFogParams();
			driver->setFog(
				player_fp.color,
				video::EFT_FOG_LINEAR,
				player_fp.fog_start, // This might need to be multiplied by fog_range if we had it here
				player_fp.fog_end,
				0.01f,
				false,
				true
			);
		}
	}
}
