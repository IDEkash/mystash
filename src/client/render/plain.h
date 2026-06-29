// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2017 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

#pragma once
#include "core.h"
#include "pipeline.h"

namespace irr {
	namespace core { template <class T> class rect; }
	namespace scene { class ICameraSceneNode; }
}

/**
 * Implements a pipeline step that renders the 3D scene
 */
class Draw3D : public RenderStep
{
public:
	virtual void setRenderSource(RenderSource *source) override {}
	virtual void setRenderTarget(RenderTarget *target) override { m_target = target; }

	virtual void reset(PipelineContext &context) override {}
	virtual void run(PipelineContext &context) override;

	void setCamera(irr::scene::ICameraSceneNode *camera) { m_camera = camera; }
	void setViewport(const irr::core::rect<irr::s32> &viewport);

private:
	RenderTarget *m_target {nullptr};
	irr::scene::ICameraSceneNode *m_camera {nullptr};
	irr::s32 m_vx1 {0}, m_vy1 {0}, m_vx2 {0}, m_vy2 {0};
	bool m_has_viewport {false};
};

class DrawWield : public RenderStep
{
public:
	virtual void setRenderSource(RenderSource *) override {}
	virtual void setRenderTarget(RenderTarget *target) override { m_target = target; }

	virtual void reset(PipelineContext &context) override {}
	virtual void run(PipelineContext &context) override;

private:
	RenderTarget *m_target {nullptr};
};

/**
 * Implements a pipeline step that renders the game HUD
 */
class DrawHUD : public RenderStep
{
public:
	virtual void setRenderSource(RenderSource *) override {}
	virtual void setRenderTarget(RenderTarget *) override {}

	virtual void reset(PipelineContext &context) override {}
	virtual void run(PipelineContext &context) override;
};

class MapPostFxStep : public TrivialRenderStep
{
public:
	virtual void setRenderTarget(RenderTarget *) override;
	virtual void run(PipelineContext &context) override;
private:
	RenderTarget *target;
};

class RenderShadowMapStep : public TrivialRenderStep
{
public:
	virtual void run(PipelineContext &context) override;
};

/**
 * UpscaleStep step performs rescaling of the image
 * in the source texture 0 to the size of the target.
 */
class UpscaleStep : public RenderStep
{
public:

	virtual void setRenderSource(RenderSource *source) override { m_source = source; }
	virtual void setRenderTarget(RenderTarget *target) override { m_target = target; }
	virtual void reset(PipelineContext &context) override {};
	virtual void run(PipelineContext &context) override;
private:
	RenderSource *m_source;
	RenderTarget *m_target;
};

std::unique_ptr<RenderStep> create3DStage(Client *client, v2f scale);
RenderStep* addUpscaling(RenderPipeline *pipeline, RenderStep *previousStep, v2f downscale_factor, Client *client);

void populatePlainPipeline(RenderPipeline *pipeline, Client *client);

video::ECOLOR_FORMAT selectColorFormat(video::IVideoDriver *driver);
video::ECOLOR_FORMAT selectDepthFormat(video::IVideoDriver *driver);
