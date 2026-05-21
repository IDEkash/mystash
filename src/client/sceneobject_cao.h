// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "clientobject.h"
#include "content_cao.h"
#include <vector>

namespace scene {
	class SMeshBuffer;
	class SMesh;
	class IMeshSceneNode;
}

class SceneObjectCAO : public ClientActiveObject
{
public:
	SceneObjectCAO(u16 id, Client *client, ClientEnvironment *env);
	virtual ~SceneObjectCAO();

	ActiveObjectType getType() const override { return ACTIVEOBJECT_TYPE_SCENEOBJECT; }

	void addToScene(ITextureSource *tsrc, scene::ISceneManager *smgr) override;
	void removeFromScene(bool permanent) override;
	void updateLight(u32 day_night_ratio) override;

	bool getCollisionBox(aabb3f *toset) const override;
	bool getSelectionBox(aabb3f *toset) const override;

	const v3f getPosition() const override { return m_position; }
	scene::ISceneNode *getSceneNode() const override;

	void step(float dtime, ClientEnvironment *env) override;
	void processMessage(const std::string &data) override;
	void initialize(const std::string &data) override;

	static ClientActiveObject* create(Client *client, ClientEnvironment *env);

private:
	void updateMesh();
	void updateMaterial();
	void updateNodePos();
	void updateAttachments();

	v3f m_position;
	v3f m_rotation;
	v3f m_scale = v3f(1, 1, 1);

	// Smoothing
	SmoothTranslator<v3f> pos_translator;
	SmoothTranslator<v3f> rot_translator;
	SmoothTranslator<v3f> scale_translator;

	// Mesh data
	std::vector<v3f> m_vertices;
	std::vector<u16> m_faces;
	std::vector<v2f> m_uvs;

	// Visual data
	std::string m_texture_name = "no_texture.png";
	video::SColor m_color = video::SColor(255, 255, 255, 255);
	video::SColor m_last_light = video::SColor(255, 255, 255, 255);

	// Irrlicht nodes
	scene::ISceneManager *m_smgr = nullptr;
	scene::SMeshBuffer *m_mesh_buffer = nullptr;
	scene::SMesh *m_mesh = nullptr;
	scene::IMeshSceneNode *m_node = nullptr;

	// Hierarchy
	u16 m_attachment_parent_id = 0;
	std::string m_attachment_bone = "";
	v3f m_attachment_position;
	v3f m_attachment_rotation;
	bool m_force_visible = false;
};
