#pragma once

#include "irr_v3d.h"
#include "irrlichttypes.h"
#include <map>
#include <mutex>
#include <vector>
#include "util/thread.h"

namespace scene {
	class IMesh;
	class IMeshBuffer;
	class ISceneManager;
}

class Client;

class LandscapePreview {
public:
	LandscapePreview(Client *client);
	~LandscapePreview();

	scene::IMesh* getOrCreateMesh(v3s16 blockpos);
	void clear();
	void step(float dtime, v3f player_pos);

private:
	struct MeshData {
		std::vector<video::S3DVertex> vertices;
		std::vector<u16> indices;
	};

	class PreviewWorker : public Thread {
	public:
		PreviewWorker(LandscapePreview *parent) : Thread("PreviewWorker"), m_parent(parent) {}
		void *run();
	private:
		LandscapePreview *m_parent;
	};

	scene::IMesh* createMeshFromData(const MeshData &data);

	Client *m_client;
	std::map<v3s16, scene::IMesh*> m_meshes;

	std::mutex m_queue_mutex;
	std::vector<v3s16> m_queue;
	std::map<v3s16, MeshData> m_finished_data;

	u64 m_seed;
	float m_clear_timer = 0.0f;
	PreviewWorker *m_worker;
};
