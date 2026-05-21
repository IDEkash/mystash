// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "sceneobject_sao.h"
#include "serverenvironment.h"
#include "util/serialize.h"
#include "constants.h"
#include "log.h"
#include "scripting_server.h"

SceneObjectSAO::SceneObjectSAO(ServerEnvironment *env, v3f pos) :
	ServerActiveObject(env, pos),
	m_guid(env->getGUIDGenerator().next())
{
}

SceneObjectSAO::~SceneObjectSAO()
{
	clearChildAttachments();
}

void SceneObjectSAO::step(float dtime, bool send_recommended)
{
	m_env->getScriptIface()->sceneobject_step(m_id, dtime);

	if (m_auto_sync && send_recommended) {
		sync();
	}
}

std::string SceneObjectSAO::getClientInitializationData(u16 protocol_version)
{
	std::ostringstream os(std::ios::binary);
	writeU8(os, 1); // version
	writeV3F32(os, getBasePosition());
	writeV3F32(os, m_rotation);
	writeV3F32(os, m_scale);
	os << serializeString16(m_texture);
	writeARGB8(os, m_color);

	// Mesh data
	writeU32(os, m_vertices.size());
	for (const auto &v : m_vertices) writeV3F32(os, v);
	writeU32(os, m_faces.size());
	for (auto f : m_faces) writeU16(os, f);
	writeU32(os, m_uvs.size());
	for (const auto &uv : m_uvs) writeV2F32(os, uv);

	// Attachment
	writeU16(os, m_attachment_parent_id);
	os << serializeString16(m_attachment_bone);
	writeV3F32(os, m_attachment_position);
	writeV3F32(os, m_attachment_rotation);
	writeU8(os, m_force_visible);

	return os.str();
}

void SceneObjectSAO::getStaticData(std::string *result) const
{
	std::ostringstream os(std::ios::binary);
	writeU8(os, 1); // version
	writeV3F32(os, getBasePosition());
	writeV3F32(os, m_rotation);
	writeV3F32(os, m_scale);
	os << serializeString16(m_texture);
	writeARGB8(os, m_color);

	writeU32(os, m_vertices.size());
	for (const auto &v : m_vertices) writeV3F32(os, v);
	writeU32(os, m_faces.size());
	for (auto f : m_faces) writeU16(os, f);
	writeU32(os, m_uvs.size());
	for (const auto &uv : m_uvs) writeV2F32(os, uv);

	m_guid.serialize(os);
	*result = os.str();
}

void SceneObjectSAO::setPos(const v3f &pos)
{
	setBasePosition(pos);
	m_transform_dirty = true;
	m_smooth_pos.active = false;
}

void SceneObjectSAO::setRot(v3f rot)
{
	m_rotation = rot;
	m_transform_dirty = true;
	m_smooth_rot.active = false;
}

void SceneObjectSAO::setScale(v3f scale)
{
	m_scale = scale;
	m_transform_dirty = true;
	m_smooth_scale.active = false;
}

void SceneObjectSAO::setPosSmooth(v3f pos, float time)
{
	m_smooth_pos.target = pos;
	m_smooth_pos.time = time;
	m_smooth_pos.active = true;
	m_transform_dirty = true;
}

void SceneObjectSAO::setRotSmooth(v3f rot, float time)
{
	m_smooth_rot.target = rot;
	m_smooth_rot.time = time;
	m_smooth_rot.active = true;
	m_transform_dirty = true;
}

void SceneObjectSAO::setScaleSmooth(v3f scale, float time)
{
	m_smooth_scale.target = scale;
	m_smooth_scale.time = time;
	m_smooth_scale.active = true;
	m_transform_dirty = true;
}

void SceneObjectSAO::cancelSmooth(const std::string &prop)
{
	if (prop == "pos") m_smooth_pos.active = false;
	else if (prop == "rot") m_smooth_rot.active = false;
	else if (prop == "scale") m_smooth_scale.active = false;
	m_transform_dirty = true;
}

void SceneObjectSAO::setVertices(const std::vector<v3f> &vertices)
{
	m_vertices = vertices;
	m_mesh_dirty = true;
	updateCollisionBox();
}

void SceneObjectSAO::setFaces(const std::vector<u16> &faces)
{
	m_faces = faces;
	m_mesh_dirty = true;
}

void SceneObjectSAO::setVertex(u32 index, v3f pos)
{
	if (index < m_vertices.size()) {
		m_vertices[index] = pos;
		if (m_auto_sync) {
			std::ostringstream os(std::ios::binary);
			writeU8(os, AO_CMD_SOS_SET_VERTEX);
			writeU32(os, index);
			writeV3F32(os, pos);
			m_messages_out.emplace(m_id, true, os.str());
		} else {
			m_mesh_dirty = true;
		}
		updateCollisionBox();
	}
}

void SceneObjectSAO::setVertexUV(u32 index, v2f uv)
{
	if (index >= m_uvs.size())
		m_uvs.resize(index + 1, v2f(0, 0));
	m_uvs[index] = uv;
	m_mesh_dirty = true;
}

void SceneObjectSAO::setUV(const std::vector<v2f> &uvs)
{
	m_uvs = uvs;
	m_mesh_dirty = true;
}

void SceneObjectSAO::setTexture(const std::string &texture)
{
	m_texture = texture;
	m_visual_dirty = true;
}

void SceneObjectSAO::setColor(video::SColor color)
{
	m_color = color;
	m_visual_dirty = true;
}

void SceneObjectSAO::setAttachment(object_t parent_id, const std::string &bone,
		v3f position, v3f rotation, bool force_visible)
{
	if (parent_id == m_id) return;

	if (m_attachment_parent_id) {
		if (auto *old_parent = getParent())
			old_parent->removeAttachmentChild(m_id);
	}

	m_attachment_parent_id = parent_id;
	m_attachment_bone = bone;
	m_attachment_position = position;
	m_attachment_rotation = rotation;
	m_force_visible = force_visible;

	if (m_attachment_parent_id) {
		if (auto *new_parent = getParent())
			new_parent->addAttachmentChild(m_id);
	}

	m_transform_dirty = true;
}

void SceneObjectSAO::getAttachment(object_t *parent_id, std::string *bone, v3f *position,
		v3f *rotation, bool *force_visible) const
{
	*parent_id = m_attachment_parent_id;
	*bone = m_attachment_bone;
	*position = m_attachment_position;
	*rotation = m_attachment_rotation;
	*force_visible = m_force_visible;
}

void SceneObjectSAO::clearChildAttachments()
{
	while (!m_attachment_child_ids.empty()) {
		object_t child_id = *m_attachment_child_ids.begin();
		if (auto *child = m_env->getActiveObject(child_id))
			child->clearParentAttachment();
		else
			m_attachment_child_ids.erase(child_id);
	}
}

void SceneObjectSAO::addAttachmentChild(object_t child_id)
{
	m_attachment_child_ids.insert(child_id);
}

void SceneObjectSAO::removeAttachmentChild(object_t child_id)
{
	m_attachment_child_ids.erase(child_id);
}

ServerActiveObject *SceneObjectSAO::getParent() const
{
	return m_attachment_parent_id ? m_env->getActiveObject(m_attachment_parent_id) : nullptr;
}

u32 SceneObjectSAO::punch(v3f dir, const ToolCapabilities &toolcap,
		ServerActiveObject *puncher, float time_from_last_punch, u16 initial_wear)
{
	m_env->getScriptIface()->sceneobject_on_punch(m_id, puncher,
			time_from_last_punch, toolcap, dir);
	return 0;
}

void SceneObjectSAO::rightClick(ServerActiveObject *clicker)
{
	m_env->getScriptIface()->sceneobject_on_rightclick(m_id, clicker);
}

bool SceneObjectSAO::getCollisionBox(aabb3f *toset) const
{
	*toset = m_collision_box;
	toset->MinEdge += getBasePosition();
	toset->MaxEdge += getBasePosition();
	return true;
}

bool SceneObjectSAO::getSelectionBox(aabb3f *toset) const
{
	*toset = m_collision_box;
	return true;
}

void SceneObjectSAO::sync()
{
	if (m_transform_dirty) sendTransform();
	if (m_mesh_dirty) sendMesh();
	if (m_visual_dirty) sendVisual();
}

void SceneObjectSAO::updateCollisionBox()
{
	if (m_vertices.empty()) {
		m_collision_box = aabb3f(-0.5, -0.5, -0.5, 0.5, 0.5, 0.5);
		return;
	}
	m_collision_box.reset(m_vertices[0]);
	for (size_t i = 1; i < m_vertices.size(); ++i)
		m_collision_box.addInternalPoint(m_vertices[i]);
}

void SceneObjectSAO::sendTransform()
{
	std::ostringstream os(std::ios::binary);
	writeU8(os, AO_CMD_SOS_SET_TRANSFORM);
	writeV3F32(os, getBasePosition());
	writeV3F32(os, m_rotation);
	writeV3F32(os, m_scale);

	writeU8(os, m_smooth_pos.active);
	if (m_smooth_pos.active) {
		writeV3F32(os, m_smooth_pos.target);
		writeF32(os, m_smooth_pos.time);
	}
	writeU8(os, m_smooth_rot.active);
	if (m_smooth_rot.active) {
		writeV3F32(os, m_smooth_rot.target);
		writeF32(os, m_smooth_rot.time);
	}
	writeU8(os, m_smooth_scale.active);
	if (m_smooth_scale.active) {
		writeV3F32(os, m_smooth_scale.target);
		writeF32(os, m_smooth_scale.time);
	}

	// Attachment info also sent here
	writeU16(os, m_attachment_parent_id);
	os << serializeString16(m_attachment_bone);
	writeV3F32(os, m_attachment_position);
	writeV3F32(os, m_attachment_rotation);
	writeU8(os, m_force_visible);

	m_messages_out.emplace(m_id, true, os.str());
	m_transform_dirty = false;
}

void SceneObjectSAO::sendMesh()
{
	{
		std::ostringstream os(std::ios::binary);
		writeU8(os, AO_CMD_SOS_SET_VERTICES);
		writeU32(os, m_vertices.size());
		for (const auto &v : m_vertices) writeV3F32(os, v);
		m_messages_out.emplace(m_id, true, os.str());
	}
	{
		std::ostringstream os(std::ios::binary);
		writeU8(os, AO_CMD_SOS_SET_FACES);
		writeU32(os, m_faces.size());
		for (auto f : m_faces) writeU16(os, f);
		m_messages_out.emplace(m_id, true, os.str());
	}
	{
		std::ostringstream os(std::ios::binary);
		writeU8(os, AO_CMD_SOS_SET_UV);
		writeU32(os, m_uvs.size());
		for (const auto &uv : m_uvs) writeV2F32(os, uv);
		m_messages_out.emplace(m_id, true, os.str());
	}
	m_mesh_dirty = false;
}

void SceneObjectSAO::sendVisual()
{
	std::ostringstream os(std::ios::binary);
	writeU8(os, AO_CMD_SOS_SET_VISUAL);
	os << serializeString16(m_texture);
	writeARGB8(os, m_color);
	m_messages_out.emplace(m_id, true, os.str());
	m_visual_dirty = false;
}
