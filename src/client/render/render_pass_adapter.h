// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2026 Luanti contributors

#pragma once

#include "render_interfaces.h"
#include "pipeline.h"

namespace rendering
{

/**
 * Adapter linking IRenderPass to Luanti's RenderStep architecture.
 */
class RenderPassStepAdapter : public RenderStep
{
public:
	RenderPassStepAdapter(std::shared_ptr<IRenderPass> pass, IRenderContext *context)
		: m_pass(pass), m_context(context)
	{
	}

	virtual ~RenderPassStepAdapter() override = default;

	virtual void setRenderSource(RenderSource *source) override { m_source = source; }
	virtual void setRenderTarget(RenderTarget *target) override { m_target = target; }

	virtual void run(PipelineContext &context) override
	{
		if (m_pass && m_pass->isEnabled() && m_context) {
			m_pass->prepare(m_context);
			m_pass->execute(m_context);
			m_pass->cleanup(m_context);
		}
	}

private:
	std::shared_ptr<IRenderPass> m_pass;
	IRenderContext *m_context {nullptr};
	RenderSource *m_source {nullptr};
	RenderTarget *m_target {nullptr};
};

} // namespace rendering
