// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2026 Luanti contributors

#pragma once

#include "render_interfaces.h"
#include <ISceneManager.h>
#include <memory>

namespace rendering
{

class SecondaryCameraRenderPass : public IRenderPass
{
public:
	SecondaryCameraRenderPass(const std::string &name, std::shared_ptr<ICamera> camera, std::shared_ptr<IRenderTarget> target, scene::ISceneManager *smgr)
		: m_name(name), m_camera(camera), m_target(target), m_smgr(smgr)
	{
	}

	virtual ~SecondaryCameraRenderPass() override = default;

	virtual const std::string &getName() const override { return m_name; }
	virtual bool isEnabled() const override { return m_enabled; }
	virtual void setEnabled(bool enabled) override { m_enabled = enabled; }

	virtual void prepare(IRenderContext *context) override
	{
		if (!context)
			return;

		m_prev_target = context->getRenderTarget();
		m_prev_camera = context->getCamera();

		if (m_target)
			context->setRenderTarget(m_target);
		if (m_camera)
			context->setCamera(m_camera);
	}

	virtual void execute(IRenderContext *context) override
	{
		if (m_smgr && m_enabled) {
			m_smgr->drawAll();
		}
	}

	virtual void cleanup(IRenderContext *context) override
	{
		if (!context)
			return;

		if (m_prev_target)
			context->setRenderTarget(m_prev_target);
		else if (m_target)
			m_target->unbind();

		if (m_prev_camera)
			context->setCamera(m_prev_camera);
	}

private:
	std::string m_name;
	bool m_enabled {true};
	std::shared_ptr<ICamera> m_camera;
	std::shared_ptr<IRenderTarget> m_target;
	scene::ISceneManager *m_smgr {nullptr};

	std::shared_ptr<IRenderTarget> m_prev_target;
	std::shared_ptr<ICamera> m_prev_camera;
};

class PortalRenderPass : public IRenderPass
{
public:
	PortalRenderPass(const std::string &name, std::shared_ptr<ICamera> portalCamera, std::shared_ptr<IRenderTarget> portalTarget, scene::ISceneManager *smgr, u8 maxRecursion = 2)
		: m_name(name), m_portal_camera(portalCamera), m_portal_target(portalTarget), m_smgr(smgr), m_max_recursion(maxRecursion)
	{
	}

	virtual ~PortalRenderPass() override = default;

	virtual const std::string &getName() const override { return m_name; }
	virtual bool isEnabled() const override { return m_enabled; }
	virtual void setEnabled(bool enabled) override { m_enabled = enabled; }

	virtual void prepare(IRenderContext *context) override
	{
		if (!context)
			return;

		m_prev_target = context->getRenderTarget();
		m_prev_camera = context->getCamera();

		if (m_portal_target)
			context->setRenderTarget(m_portal_target);
		if (m_portal_camera)
			context->setCamera(m_portal_camera);
	}

	virtual void execute(IRenderContext *context) override
	{
		if (m_enabled)
			renderRecursive(context, 0);
	}

	virtual void cleanup(IRenderContext *context) override
	{
		if (!context)
			return;

		if (m_prev_target)
			context->setRenderTarget(m_prev_target);
		else if (m_portal_target)
			m_portal_target->unbind();

		if (m_prev_camera)
			context->setCamera(m_prev_camera);
	}

private:
	void renderRecursive(IRenderContext *context, u8 currentDepth)
	{
		if (currentDepth > m_max_recursion || !m_smgr)
			return;

		m_smgr->drawAll();
	}

	std::string m_name;
	bool m_enabled {true};
	std::shared_ptr<ICamera> m_portal_camera;
	std::shared_ptr<IRenderTarget> m_portal_target;
	scene::ISceneManager *m_smgr {nullptr};
	u8 m_max_recursion {2};

	std::shared_ptr<IRenderTarget> m_prev_target;
	std::shared_ptr<ICamera> m_prev_camera;
};

} // namespace rendering
