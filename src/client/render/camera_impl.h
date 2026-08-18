// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2026 Luanti contributors

#pragma once

#include "render_interfaces.h"
#include <ICameraSceneNode.h>
#include <ISceneManager.h>

namespace rendering
{

class IrrlichtCamera : public ICamera
{
public:
	IrrlichtCamera(scene::ISceneManager *smgr)
		: m_smgr(smgr)
	{
		if (m_smgr) {
			m_camera_node = m_smgr->addCameraSceneNode(nullptr, v3f(0, 0, 0), v3f(0, 0, 1));
			if (m_camera_node)
				m_camera_node->grab();
		}
	}

	virtual ~IrrlichtCamera() override
	{
		if (m_camera_node) {
			m_camera_node->drop();
			m_camera_node = nullptr;
		}
	}

	virtual void setPosition(const v3f &pos) override
	{
		m_pos = pos;
		if (m_camera_node)
			m_camera_node->setPosition(pos);
	}

	virtual v3f getPosition() const override { return m_pos; }

	virtual void setTarget(const v3f &target) override
	{
		m_target = target;
		if (m_camera_node)
			m_camera_node->setTarget(target);
	}

	virtual v3f getTarget() const override { return m_target; }

	virtual void setUpVector(const v3f &up) override
	{
		m_up = up;
		if (m_camera_node)
			m_camera_node->setUpVector(up);
	}

	virtual v3f getUpVector() const override { return m_up; }

	virtual void setFOV(f32 fovDegrees) override
	{
		m_fov = fovDegrees;
		if (m_camera_node)
			m_camera_node->setFOV(fovDegrees * core::DEGTORAD);
	}

	virtual f32 getFOV() const override { return m_fov; }

	virtual void setAspectRatio(f32 aspect) override
	{
		m_aspect = aspect;
		if (m_camera_node)
			m_camera_node->setAspectRatio(aspect);
	}

	virtual f32 getAspectRatio() const override { return m_aspect; }

	virtual void setNearPlane(f32 nearVal) override
	{
		m_near = nearVal;
		if (m_camera_node)
			m_camera_node->setNearValue(nearVal);
	}

	virtual f32 getNearPlane() const override { return m_near; }

	virtual void setFarPlane(f32 farVal) override
	{
		m_far = farVal;
		if (m_camera_node)
			m_camera_node->setFarValue(farVal);
	}

	virtual f32 getFarPlane() const override { return m_far; }

	virtual const core::matrix4 &getViewMatrix() override
	{
		if (m_camera_node)
			return m_camera_node->getViewMatrix();
		return m_dummy_mat;
	}

	virtual const core::matrix4 &getProjectionMatrix() override
	{
		if (m_camera_node)
			return m_camera_node->getProjectionMatrix();
		return m_dummy_mat;
	}

	virtual core::matrix4 getViewProjectionMatrix() override
	{
		return getProjectionMatrix() * getViewMatrix();
	}

	scene::ICameraSceneNode *getIrrlichtCameraNode() const { return m_camera_node; }

private:
	scene::ISceneManager *m_smgr {nullptr};
	scene::ICameraSceneNode *m_camera_node {nullptr};
	v3f m_pos {0, 0, 0};
	v3f m_target {0, 0, 1};
	v3f m_up {0, 1, 0};
	f32 m_fov {70.0f};
	f32 m_aspect {4.0f / 3.0f};
	f32 m_near {0.1f};
	f32 m_far {1000.0f};
	core::matrix4 m_dummy_mat;
};

} // namespace rendering
