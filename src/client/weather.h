// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2024 Jules

#pragma once

#include "irrlichttypes_bloated.h"
#include <ISceneNode.h>
#include <CMeshBuffer.h>
#include "irr_ptr.h"

namespace video {
	class IVideoDriver;
}

class IShaderSource;
class ITextureSource;

enum WeatherType {
	WEATHER_NONE,
	WEATHER_RAIN,
	WEATHER_SNOW
};

class Weather : public scene::ISceneNode
{
public:
	Weather(scene::ISceneManager* mgr, ITextureSource *tsrc, IShaderSource *ssrc, s32 id);
	virtual ~Weather();

	virtual void OnRegisterSceneNode();
	virtual void render();
	virtual const aabb3f &getBoundingBox() const { return m_box; }

	void setType(WeatherType type);
	void setIntensity(float intensity);
	void update(v3f camera_pos, v3f camera_dir, float dtime);

	void setClassic(bool classic) { m_classic = classic; }

private:
	void updateMesh();

	aabb3f m_box{{-1000, -1000, -1000}, {1000, 1000, 1000}};
	video::SMaterial m_material;
	irr_ptr<scene::SMeshBuffer> m_meshbuffer;

	WeatherType m_type = WEATHER_NONE;
	float m_intensity = 0.0f;
	v3f m_camera_pos;
	v3f m_camera_dir;
	float m_timer = 0.0f;
	bool m_classic = true;

	ITextureSource *m_tsrc;
};
