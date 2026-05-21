// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "serveractiveobject.h"
#include "util/guid.h"
#include <vector>

class SceneObjectSAO : public ServerActiveObject
{
public:
	SceneObjectSAO(ServerEnvironment *env, v3f pos);
	SceneObjectSAO(ServerEnvironment *env, v3f pos, const std::string &data);
	~SceneObjectSAO();

	ActiveObjectType getType() const override { return ACTIVEOBJECT_TYPE_SCENEOBJECT; }
	ActiveObjectType getSendType() const override { return ACTIVEOBJECT_TYPE_SCENEOBJECT; }

	void removingFromEnvironment() override;

	void step(float dtime, bool send_recommended) override;
	std::string getClientInitializationData(u16 protocol_version) override;
	void getStaticData(std::string *result) const override;

	bool isStaticAllowed() const override { return true; }
	bool shouldUnload() const override { return true; }

	std::string getGUID() const override { return "@" + m_guid.base64(); }

	// Transform
	void setPos(const v3f &pos) override;
	void setRot(v3f rot);
	void setScale(v3f scale);

	v3f getRot() const { return m_rotation; }
	v3f getScale() const { return m_scale; }

	v3f getWorldPos() const;
	v3f getWorldRot() const;

	void setPosSmooth(v3f pos, float time);
	void setRotSmooth(v3f rot, float time);
	void setScaleSmooth(v3f scale, float time);
	void cancelSmooth(const std::string &prop);

	// Mesh
	void setVertices(const std::vector<v3f> &vertices);
	void setFaces(const std::vector<u16> &faces);
	const std::vector<u16>& getFaces() const { return m_faces; }
	void setVertex(u32 index, v3f pos);
	void setVertexUV(u32 index, v2f uv);
	void setUV(const std::vector<v2f> &uvs);

	// Visual
	void setTexture(const std::string &texture);
	void setColor(video::SColor color);

	// Hierarchy
	void setAttachment(object_t parent_id, const std::string &bone, v3f position,
			v3f rotation, bool force_visible) override;
	void getAttachment(object_t *parent_id, std::string *bone, v3f *position,
			v3f *rotation, bool *force_visible) const override;
	void clearChildAttachments() override;
	void addAttachmentChild(object_t child_id) override;
	void removeAttachmentChild(object_t child_id) override;
	const std::unordered_set<object_t> &getAttachmentChildIds() const override {
		return m_attachment_child_ids;
	}
	ServerActiveObject *getParent() const override;

	// Collision
	bool getCollisionBox(aabb3f *toset) const override;
	bool getSelectionBox(aabb3f *toset) const override;
	bool collideWithObjects() const override { return true; }

	// Logic
	u32 punch(v3f dir, const ToolCapabilities &toolcap,
			ServerActiveObject *puncher = nullptr,
			float time_from_last_punch = 1000000.0f,
			u16 initial_wear = 0) override;
	void rightClick(ServerActiveObject *clicker) override;

	// Networking
	void setSync(bool sync) { m_auto_sync = sync; }
	void sync();

private:
	void updateCollisionBox();
	void sendTransform();
	void sendMesh();
	void sendVisual();

	MyGUID m_guid;
	v3f m_rotation;
	v3f m_scale = v3f(1, 1, 1);

	// Smoothing
	struct SmoothTarget {
		v3f target;
		float time = 0.0f;
		bool active = false;
	} m_smooth_pos, m_smooth_rot, m_smooth_scale;

	// Mesh data
	std::vector<v3f> m_vertices;
	std::vector<u16> m_faces;
	std::vector<v2f> m_uvs;

	// Visual data
	std::string m_texture = "no_texture.png";
	video::SColor m_color = video::SColor(255, 255, 255, 255);

	// Hierarchy
	object_t m_attachment_parent_id = 0;
	std::unordered_set<object_t> m_attachment_child_ids;
	std::string m_attachment_bone = "";
	v3f m_attachment_position;
	v3f m_attachment_rotation;
	bool m_force_visible = false;

	// Collision
	aabb3f m_collision_box = aabb3f(-0.5, -0.5, -0.5, 0.5, 0.5, 0.5);

	// Networking
	bool m_auto_sync = true;
	bool m_transform_dirty = false;
	bool m_mesh_dirty = false;
	bool m_visual_dirty = false;
};
