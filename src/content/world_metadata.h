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

struct WorldDirIndexEntry {
    std::string path;
    std::string name;
};

struct WorldDirIndex {
    std::vector<WorldDirIndexEntry> worlds;

    void fromJson(const Json::Value &root);
    Json::Value toJson() const;
};

bool readWorldMetadata(const std::string &world_path, WorldMetadata &meta);
bool writeWorldMetadata(const std::string &world_path, const WorldMetadata &meta);

bool readWorldDirIndex(WorldDirIndex &index);
bool writeWorldDirIndex(const WorldDirIndex &index);
