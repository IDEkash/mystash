#pragma once

#include <string>
#include <vector>
#include <json/json.h>

struct WorldMetadata {
    std::string name;
    std::string seed;
    std::string mapgen;
    bool visible = true;
    bool hidden = false;
    std::vector<std::string> linked_mods;

    void fromJson(const Json::Value &root);
    Json::Value toJson() const;
};

struct WorldIndexEntry {
    std::string path;
    std::string name;
};

struct WorldIndex {
    std::vector<WorldIndexEntry> worlds;

    void fromJson(const Json::Value &root);
    Json::Value toJson() const;
};

bool readWorldMetadata(const std::string &world_path, WorldMetadata &meta);
bool writeWorldMetadata(const std::string &world_path, const WorldMetadata &meta);

bool readWorldIndex(WorldIndex &index);
bool writeWorldIndex(const WorldIndex &index);
