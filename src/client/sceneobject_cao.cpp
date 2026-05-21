// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "sceneobject_cao.h"
#include "client/client.h"
#include "client/texturesource.h"
#include "util/serialize.h"
#include "constants.h"
#include "log.h"
#include <SMesh.h>
#include <SMeshBuffer.h>
#include <IMeshSceneNode.h>
#include <IBoneSceneNode.h>

SceneObjectCAO::SceneObjectCAO(u16 id, Client *client, ClientEnvironment *env) :
	ClientActiveObject(id, client, env)
{
}

SceneObjectCAO::~SceneObjectCAO()
{
	removeFromScene(true);
}

ClientActiveObject* SceneObjectCAO::create(Client *client, ClientEnvironment *env)
{
	return new SceneObjectCAO(0, client, env);
}

void SceneObjectCAO::addToScene(ITextureSource *tsrc, scene::ISceneManager *smgr)
{
	m_smgr = smgr;
	if (m_node) return;

	m_mesh_buffer = new scene::SMeshBuffer();
	m_mesh = new scene::SMesh();
	m_mesh->addMeshBuffer(m_mesh_buffer);
	m_mesh_buffer->drop();

	m_node = smgr->addMeshSceneNode(m_mesh);
	m_mesh->drop();

	updateMesh();
	updateMaterial();
	updateNodePos();
	updateAttachments();
}

void SceneObjectCAO::removeFromScene(bool permanent)
{
	if (m_node) {
		m_node->remove();
		m_node = nullptr;
	}
	m_mesh = nullptr;
	m_mesh_buffer = nullptr;
}

void SceneObjectCAO::updateLight(u32 day_night_ratio)
{
	if (!m_node) return;
	v3s16 pos = floatToInt(m_position, BS);
	bool pos_ok;
	MapNode n = m_env->getMap().getNode(pos, &pos_ok);
	video::SColor color;
	if (pos_ok)
		color = encode_light(n.getLightBlend(day_night_ratio, m_client->ndef()), 0);
	else
		color = video::SColor(255, 255, 255, 255);

	if (color != m_last_light) {
		m_last_light = color;
		updateMaterial();
	}
}

bool SceneObjectCAO::getCollisionBox(aabb3f *toset) const
{
	if (!m_mesh) return false;
	*toset = m_mesh->getBoundingBox();
	toset->MinEdge += m_position;
	toset->MaxEdge += m_position;
	return true;
}

bool SceneObjectCAO::getSelectionBox(aabb3f *toset) const
{
	if (!m_mesh) return false;
	*toset = m_mesh->getBoundingBox();
	return true;
}

scene::ISceneNode *SceneObjectCAO::getSceneNode() const
{
	return m_node;
}

void SceneObjectCAO::step(float dtime, ClientEnvironment *env)
{
	pos_translator.translate(dtime);
	rot_translator.translate(dtime);
	scale_translator.translate(dtime);

	m_position = pos_translator.val_current;
	m_rotation = rot_translator.val_current;
	m_scale = scale_translator.val_current;

	updateNodePos();
}

void SceneObjectCAO::processMessage(const std::string &data)
{
	std::istringstream is(data, std::ios::binary);
	u8 cmd = readU8(is);

	if (cmd == AO_CMD_SOS_SET_VERTICES) {
		u32 count = readU32(is);
		m_vertices.clear();
		for (u32 i = 0; i < count; ++i) m_vertices.push_back(readV3F32(is));
		updateMesh();
	} else if (cmd == AO_CMD_SOS_SET_FACES) {
		u32 count = readU32(is);
		m_faces.clear();
		for (u32 i = 0; i < count; ++i) m_faces.push_back(readU16(is));
		updateMesh();
	} else if (cmd == AO_CMD_SOS_SET_UV) {
		u32 count = readU32(is);
		m_uvs.clear();
		for (u32 i = 0; i < count; ++i) m_uvs.push_back(readV2F32(is));
		updateMesh();
	} else if (cmd == AO_CMD_SOS_SET_VERTEX) {
		u32 index = readU32(is);
		v3f pos = readV3F32(is);
		if (index < m_vertices.size()) {
			m_vertices[index] = pos;
			updateMesh();
		}
	} else if (cmd == AO_CMD_SOS_SET_VISUAL) {
		m_texture_name = deSerializeString16(is);
		m_color = readARGB8(is);
		updateMaterial();
	} else if (cmd == AO_CMD_SOS_SET_TRANSFORM) {
		v3f pos = readV3F32(is);
		v3f rot = readV3F32(is);
		v3f scale = readV3F32(is);

		bool smooth_pos = readU8(is);
		if (smooth_pos) {
			v3f target = readV3F32(is);
			float time = readF32(is);
			pos_translator.update(target, true, time);
		} else {
			pos_translator.init(pos);
		}

		bool smooth_rot = readU8(is);
		if (smooth_rot) {
			v3f target = readV3F32(is);
			float time = readF32(is);
			rot_translator.update(target, true, time);
		} else {
			rot_translator.init(rot);
		}

		bool smooth_scale = readU8(is);
		if (smooth_scale) {
			v3f target = readV3F32(is);
			float time = readF32(is);
			scale_translator.update(target, true, time);
		} else {
			scale_translator.init(scale);
		}

		m_attachment_parent_id = readU16(is);
		m_attachment_bone = deSerializeString16(is);
		m_attachment_position = readV3F32(is);
		m_attachment_rotation = readV3F32(is);
		m_force_visible = readU8(is);
		updateAttachments();
	}
}

void SceneObjectCAO::initialize(const std::string &data)
{
	std::istringstream is(data, std::ios::binary);
	u8 version = readU8(is);
	if (version != 1) return;

	v3f pos = readV3F32(is);
	v3f rot = readV3F32(is);
	v3f scale = readV3F32(is);
	m_texture_name = deSerializeString16(is);
	m_color = readARGB8(is);

	pos_translator.init(pos);
	rot_translator.init(rot);
	scale_translator.init(scale);
	m_position = pos;
	m_rotation = rot;
	m_scale = scale;

	u32 v_count = readU32(is);
	for (u32 i = 0; i < v_count; ++i) m_vertices.push_back(readV3F32(is));
	u32 f_count = readU32(is);
	for (u32 i = 0; i < f_count; ++i) m_faces.push_back(readU16(is));
	u32 uv_count = readU32(is);
	for (u32 i = 0; i < uv_count; ++i) m_uvs.push_back(readV2F32(is));

	m_attachment_parent_id = readU16(is);
	m_attachment_bone = deSerializeString16(is);
	m_attachment_position = readV3F32(is);
	m_attachment_rotation = readV3F32(is);
	m_force_visible = readU8(is);
}

void SceneObjectCAO::updateMesh()
{
	if (!m_mesh_buffer) return;

	m_mesh_buffer->Vertices.set_used(m_vertices.size());
	for (u32 i = 0; i < m_vertices.size(); ++i) {
		m_mesh_buffer->Vertices[i].Pos = m_vertices[i];
		m_mesh_buffer->Vertices[i].Normal = v3f(0, 1, 0); // Default normal
		m_mesh_buffer->Vertices[i].Color = video::SColor(255, 255, 255, 255);
		if (i < m_uvs.size())
			m_mesh_buffer->Vertices[i].TCoords = m_uvs[i];
		else
			m_mesh_buffer->Vertices[i].TCoords = v2f(0, 0);
	}

	m_mesh_buffer->Indices.set_used(m_faces.size());
	for (u32 i = 0; i < m_faces.size(); ++i) {
		m_mesh_buffer->Indices[i] = m_faces[i];
	}

	m_mesh_buffer->recalculateBoundingBox();
	m_mesh->recalculateBoundingBox();
	if (m_node) m_node->setMesh(m_mesh);
}

void SceneObjectCAO::updateMaterial()
{
	if (!m_node) return;
	video::SMaterial &mat = m_node->getMaterial(0);
	mat.setTexture(0, m_client->tsrc()->getTexture(m_texture_name));
	mat.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
	mat.Lighting = true;
	mat.BackfaceCulling = false;

	video::SColor final_color = m_color;
	final_color.setRed((u32)m_color.getRed() * m_last_light.getRed() / 255);
	final_color.setGreen((u32)m_color.getGreen() * m_last_light.getGreen() / 255);
	final_color.setBlue((u32)m_color.getBlue() * m_last_light.getBlue() / 255);

	for (u32 i = 0; i < m_mesh_buffer->Vertices.size(); ++i) {
		m_mesh_buffer->Vertices[i].Color = final_color;
	}
}

void SceneObjectCAO::updateNodePos()
{
	if (!m_node || m_attachment_parent_id != 0) return;
	v3s16 camera_offset = m_env->getCameraOffset();
	m_node->setPosition(m_position - intToFloat(camera_offset, BS));
	m_node->setRotation(m_rotation);
	m_node->setScale(m_scale);
}

void SceneObjectCAO::updateAttachments()
{
	if (!m_node) return;
	ClientActiveObject *parent = m_env->getActiveObject(m_attachment_parent_id);
	if (parent && parent->getSceneNode()) {
		scene::ISceneNode *parent_node = parent->getSceneNode();
		if (!m_attachment_bone.empty()) {
			scene::AnimatedMeshSceneNode *anode = parent->getAnimatedMeshSceneNode();
			if (anode) {
				scene::IBoneSceneNode *bone = anode->getJointNode(m_attachment_bone.c_str());
				if (bone) parent_node = (scene::ISceneNode*)bone;
			}
		}
		m_node->setParent(parent_node);
		m_node->setPosition(m_attachment_position);
		m_node->setRotation(m_attachment_rotation);
		m_node->setScale(m_scale);
	} else {
		m_node->setParent(m_smgr->getRootSceneNode());
		updateNodePos();
	}
}

// Prototype
static SceneObjectCAO proto_SceneObjectCAO(0, nullptr, nullptr);
