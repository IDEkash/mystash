#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include "irr_v2d.h"
#include "SColor.h"

struct CCIConnection {
	std::string p1;
	std::string p2;
	float bend = 0.0f;
};

struct CCIImage {
	std::string texture;
	v2f position = v2f(0.0f, 0.0f);
	float size = 1.0f;
};

struct CCIStyle {
	std::string name;
	std::unordered_map<std::string, v2f> points;
	std::vector<CCIConnection> shape;
	video::SColor fill_color = video::SColor(255, 255, 255, 255);
	bool has_fill = false;
	float opacity = 1.0f;
	bool has_image = false;
	CCIImage image;
};

struct CCIInstance {
	std::string name;
	std::string style_name;
	v2f position = v2f(0.0f, 0.0f);
	u16 layer = 1;
	std::string player; // empty if global, otherwise specific to a player name
	u32 creation_id = 0; // for deterministic sorting on same layer

	bool operator<(const CCIInstance &other) const {
		if (layer != other.layer) {
			return layer > other.layer; // Layer 5 is larger than Layer 1, so it comes first in sorted order (drawn bottom)
		}
		return creation_id < other.creation_id;
	}
};

class CCIManager {
public:
	CCIManager() = default;
	~CCIManager() = default;

	void registerStyle(const CCIStyle &style);
	const CCIStyle* getStyle(const std::string &name) const;
	std::unordered_map<std::string, CCIStyle> getStyles() const;

	void registerInstance(const CCIInstance &instance);
	void destroyInstance(const std::string &name);
	std::unordered_map<std::string, CCIInstance> getInstances() const;
	void clear();

private:
	std::unordered_map<std::string, CCIStyle> m_styles;
	std::unordered_map<std::string, CCIInstance> m_instances;
	u32 m_next_creation_id = 0;
	mutable std::mutex m_mutex;
};
