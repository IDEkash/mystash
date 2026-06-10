// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "client/render/pipeline.h"

class RenderTarget;

/**
 * A render step that integrates VisualsService scenes into the render pipeline.
 */
class VisualsRenderStep : public TrivialRenderStep
{
public:
	virtual void setRenderTarget(RenderTarget *target) override { m_target = target; }
	virtual void run(PipelineContext &context) override;

private:
	RenderTarget *m_target = nullptr;
};
