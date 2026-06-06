// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2024 Jules

#include "weather.h"
#include "client/renderingengine.h"
#include "client/shader.h"
#include "client/texturesource.h"
#include "settings.h"
#include "profiler.h"
#include "noise.h"

Weather::Weather(scene::ISceneManager* mgr, ITextureSource *tsrc, IShaderSource *ssrc, s32 id) :
	scene::ISceneNode(mgr->getRootSceneNode(), mgr, id),
	m_tsrc(tsrc)
{
	m_material.BackfaceCulling = false;
	m_material.FogEnable = true;
	m_material.ZWriteEnable = video::EZW_OFF;
	m_material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;

	m_meshbuffer.reset(new scene::SMeshBuffer());
	m_meshbuffer->setHardwareMappingHint(scene::EHM_DYNAMIC);

	setAutomaticCulling(scene::EAC_OFF);
}

Weather::~Weather()
{
}

void Weather::OnRegisterSceneNode()
{
	if (IsVisible && m_type != WEATHER_NONE && m_intensity > 0)
		SceneManager->registerNodeForRendering(this, scene::ESNRP_TRANSPARENT);

	ISceneNode::OnRegisterSceneNode();
}

void Weather::render()
{
	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	if (!driver)
		return;

	driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);

	float scroll_speed = (m_type == WEATHER_RAIN) ? 4.0f : 1.0f;
	float t = m_timer * scroll_speed;

	core::matrix4 texMatrix;
	texMatrix.setTextureTranslate(0, t);
	m_material.setTextureMatrix(0, texMatrix);

	driver->setMaterial(m_material);
	driver->drawMeshBuffer(m_meshbuffer.get());
}

void Weather::setType(WeatherType type)
{
	if (m_type == type)
		return;

	m_type = type;
	if (m_type == WEATHER_RAIN) {
		m_material.setTexture(0, m_tsrc->getTexture("rain.png"));
	} else if (m_type == WEATHER_SNOW) {
		m_material.setTexture(0, m_tsrc->getTexture("snow.png"));
	} else {
		m_material.setTexture(0, nullptr);
	}
	updateMesh();
}

void Weather::setIntensity(float intensity)
{
	if (m_intensity == intensity)
		return;
	m_intensity = intensity;
	updateMesh();
}

void Weather::update(v3f camera_pos, v3f camera_dir, float dtime)
{
	m_camera_pos = camera_pos;
	m_camera_dir = camera_dir;
	m_timer += dtime;

	setPosition(m_camera_pos);
	updateAbsolutePosition();
}

void Weather::updateMesh()
{
	auto &vertices = m_meshbuffer->Vertices->Data;
	auto &indices = m_meshbuffer->Indices->Data;

	vertices.clear();
	indices.clear();

	if (m_type == WEATHER_NONE || m_intensity <= 0) {
		m_meshbuffer->setDirty();
		return;
	}

	float radius = 20.0f;
	int count = 8;
	float height = 20.0f;

	video::SColor color(128 * m_intensity, 255, 255, 255);

	PcgRandom rgen(42);

	for (int x = -count; x <= count; x++) {
		for (int z = -count; z <= count; z++) {
			if (x*x + z*z > count*count) continue;

			float fx = x * (radius / count);
			float fz = z * (radius / count);

			fx += (float)rgen.range(-50, 50) / 100.0f;
			fz += (float)rgen.range(-50, 50) / 100.0f;

			float v_offset = (float)rgen.range(0, 100) / 100.0f;

			u32 idx = vertices.size();

			for (int r = 0; r < 2; r++) {
				v3f p1(-0.5, -height/2, 0);
				v3f p2( 0.5, -height/2, 0);
				v3f p3( 0.5,  height/2, 0);
				v3f p4(-0.5,  height/2, 0);

				if (r == 1) {
					p1.rotateXZBy(90);
					p2.rotateXZBy(90);
					p3.rotateXZBy(90);
					p4.rotateXZBy(90);
				}

				v3f offset(fx, 0, fz);

				vertices.push_back(video::S3DVertex(p1 + offset, v3f(0,0,1), color, v2f(0, v_offset + 1)));
				vertices.push_back(video::S3DVertex(p2 + offset, v3f(0,0,1), color, v2f(1, v_offset + 1)));
				vertices.push_back(video::S3DVertex(p3 + offset, v3f(0,0,1), color, v2f(1, v_offset)));
				vertices.push_back(video::S3DVertex(p4 + offset, v3f(0,0,1), color, v2f(0, v_offset)));

				indices.push_back(idx + 0);
				indices.push_back(idx + 1);
				indices.push_back(idx + 2);
				indices.push_back(idx + 2);
				indices.push_back(idx + 3);
				indices.push_back(idx + 0);
				idx += 4;
			}
		}
	}

	m_meshbuffer->setDirty();
}
