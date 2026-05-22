#include "landscape_preview.h"
#include "client/client.h"
#include "noise.h"
#include "constants.h"
#include "client/tile.h"
#include "nodedef.h"
#include "script/scripting_client.h"
#include <SMesh.h>
#include <SMeshBuffer.h>
#include <cmath>

LandscapePreview::LandscapePreview(Client *client) : m_client(client)
{
	m_seed = client->getMapSeed();
	m_worker = new PreviewWorker(this);
	m_worker->start();
}

LandscapePreview::~LandscapePreview()
{
	m_worker->stop();
	m_worker->wait();
	delete m_worker;
	clear();
}

void LandscapePreview::clear()
{
	for (auto &it : m_meshes) {
		it.second->drop();
	}
	m_meshes.clear();
}

void LandscapePreview::step(float dtime, v3f player_pos)
{
	v3s16 p_block = getContainerPos(floatToInt(player_pos, BS), MAP_BLOCKSIZE);

	// Fetch heightmaps from Lua for nearby blocks
	for (s16 z = -2; z <= 2; z++)
	for (s16 x = -2; x <= 2; x++) {
		v3s16 bp = p_block + v3s16(x, 0, z);
		bp.Y = 0; // We only care about 2D heightmap for now

		if (m_heightmaps.find(bp) == m_heightmaps.end()) {
			std::vector<float> hmap;
			hmap.reserve(MAP_BLOCKSIZE * MAP_BLOCKSIZE);
			bool any_valid = false;

			for (s16 lz = 0; lz < MAP_BLOCKSIZE; lz++)
			for (s16 lx = 0; lx < MAP_BLOCKSIZE; lx++) {
				float h = m_client->getScript()->on_shadow_generate(
					bp, bp.X * MAP_BLOCKSIZE + lx, bp.Z * MAP_BLOCKSIZE + lz);
				hmap.push_back(h);
				if (!std::isnan(h))
					any_valid = true;
			}

			if (any_valid)
				m_heightmaps[bp] = std::move(hmap);
			else
				m_heightmaps[bp] = {}; // Mark as processed but empty
		}
	}

	m_clear_timer += dtime;
	if (m_clear_timer > 10.0f) {
		m_clear_timer = 0.0f;

		// Prune cache based on distance
		v3s16 p_block = getContainerPos(floatToInt(player_pos, BS), MAP_BLOCKSIZE);
		const float prune_dist_sq = 20 * 20;

		for (auto it = m_meshes.begin(); it != m_meshes.end(); ) {
			if (it->first.getDistanceFromSQ(p_block) > prune_dist_sq) {
				it->second->drop();
				it = m_meshes.erase(it);
			} else {
				++it;
			}
		}

		std::lock_guard<std::mutex> lock(m_queue_mutex);
		for (auto it = m_heightmaps.begin(); it != m_heightmaps.end(); ) {
			if (it->first.getDistanceFromSQ(p_block) > prune_dist_sq) {
				it = m_heightmaps.erase(it);
			} else {
				++it;
			}
		}
	}

	// Move finished data into Irrlicht meshes on main thread
	std::lock_guard<std::mutex> lock(m_queue_mutex);
	for (auto it = m_finished_data.begin(); it != m_finished_data.end(); ++it) {
		m_meshes[it->first] = createMeshFromData(it->second);
	}
	m_finished_data.clear();
}

scene::IMesh* LandscapePreview::getOrCreateMesh(v3s16 blockpos)
{
	auto it = m_meshes.find(blockpos);
	if (it != m_meshes.end())
		return it->second;

	// Queue for generation if not already queued
	std::lock_guard<std::mutex> lock(m_queue_mutex);
	if (std::find(m_queue.begin(), m_queue.end(), blockpos) == m_queue.end()) {
		m_queue.push_back(blockpos);
	}

	return nullptr;
}

scene::IMesh* LandscapePreview::createMeshFromData(const MeshData &data)
{
	auto mesh = new scene::SMesh();
	if (data.vertices.empty())
		return mesh;

	auto buf = new scene::SMeshBuffer();
	buf->append(data.vertices.data(), data.vertices.size(), data.indices.data(), data.indices.size());
	buf->recalculateBoundingBox();
	mesh->addMeshBuffer(buf);
	buf->drop();
	mesh->recalculateBoundingBox();

	return mesh;
}

void *LandscapePreview::PreviewWorker::run()
{
	while (!stopRequested()) {
		v3s16 blockpos;
		bool have_item = false;
		{
			std::lock_guard<std::mutex> lock(m_parent->m_queue_mutex);
			if (!m_parent->m_queue.empty()) {
				blockpos = m_parent->m_queue.back();
				m_parent->m_queue.pop_back();
				have_item = true;
			}
		}

		if (!have_item) {
			sleep_ms(10);
			continue;
		}

		MeshData data;
		v3s16 p0 = blockpos * MAP_BLOCKSIZE;
		NoiseParams np_terrain(0, 40, v3f(500, 500, 500), m_parent->m_seed, 5, 0.6, 2.0);
		video::SColor color(255, 100, 150, 100);

		v3s16 hbp = blockpos;
		hbp.Y = 0;
		std::vector<float> hmap;
		{
			std::lock_guard<std::mutex> lock(m_parent->m_queue_mutex);
			auto it = m_parent->m_heightmaps.find(hbp);
			if (it != m_parent->m_heightmaps.end())
				hmap = it->second;
		}

		for (s16 z = 0; z < MAP_BLOCKSIZE; z += 2)
		for (s16 x = 0; x < MAP_BLOCKSIZE; x += 2) {
			float height;
			if (!hmap.empty()) {
				height = hmap[z * MAP_BLOCKSIZE + x];
			} else {
				float world_x = p0.X + x;
				float world_z = p0.Z + z;
				height = NoiseFractal2D(&np_terrain, world_x, world_z, m_parent->m_seed);
			}

			if (!std::isnan(height) && height >= p0.Y && height < p0.Y + MAP_BLOCKSIZE) {
				float local_y = height - p0.Y;
				u32 vidx = data.vertices.size();
				v3f p(x * BS, local_y * BS, z * BS);

				data.vertices.push_back(video::S3DVertex(p.X, p.Y, p.Z, 0,1,0, color, 0,0));
				data.vertices.push_back(video::S3DVertex(p.X + 2*BS, p.Y, p.Z, 0,1,0, color, 1,0));
				data.vertices.push_back(video::S3DVertex(p.X + 2*BS, p.Y, p.Z + 2*BS, 0,1,0, color, 1,1));
				data.vertices.push_back(video::S3DVertex(p.X, p.Y, p.Z + 2*BS, 0,1,0, color, 0,1));

				data.indices.push_back(vidx + 0);
				data.indices.push_back(vidx + 1);
				data.indices.push_back(data.indices.back()); // dummy
				data.indices.back() = vidx + 2;
				data.indices.push_back(vidx + 2);
				data.indices.push_back(vidx + 3);
				data.indices.push_back(vidx + 0);
			}
		}

		{
			std::lock_guard<std::mutex> lock(m_parent->m_queue_mutex);
			m_parent->m_finished_data[blockpos] = std::move(data);
		}
	}
	return nullptr;
}
