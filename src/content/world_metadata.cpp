#include "world_metadata.h"
#include "filesys.h"
#include "porting.h"
#include "log.h"
#include <fstream>
#include <json/json.h>

void WorldMetadata::fromJson(const Json::Value &root) {
    name = root.get("name", "").asString();
    seed = root.get("seed", "").asString();
    mapgen = root.get("mapgen", "").asString();
    visible = root.get("visible", true).asBool();
    hidden = root.get("hidden", false).asBool();
    if (root.isMember("linked_mods") && root["linked_mods"].isArray()) {
        linked_mods.clear();
        for (const auto &mod : root["linked_mods"]) {
            linked_mods.push_back(mod.asString());
        }
    }
}

Json::Value WorldMetadata::toJson() const {
    Json::Value root;
    root["name"] = name;
    root["seed"] = seed;
    root["mapgen"] = mapgen;
    root["visible"] = visible;
    root["hidden"] = hidden;
    Json::Value mods(Json::arrayValue);
    for (const auto &mod : linked_mods) {
        mods.append(mod);
    }
    root["linked_mods"] = mods;
    return root;
}

void WorldIndex::fromJson(const Json::Value &root) {
    if (root.isMember("worlds") && root["worlds"].isArray()) {
        worlds.clear();
        for (const auto &w : root["worlds"]) {
            WorldIndexEntry entry;
            entry.path = w.get("path", "").asString();
            entry.name = w.get("name", "").asString();
            worlds.push_back(entry);
        }
    }
}

Json::Value WorldIndex::toJson() const {
    Json::Value root;
    Json::Value worlds_arr(Json::arrayValue);
    for (const auto &w : worlds) {
        Json::Value entry;
        entry["path"] = w.path;
        entry["name"] = w.name;
        worlds_arr.append(entry);
    }
    root["worlds"] = worlds_arr;
    return root;
}

bool readWorldMetadata(const std::string &world_path, WorldMetadata &meta) {
    std::string path = world_path + DIR_DELIM + "worldmeta.json";
    std::ifstream is(path.c_str(), std::ios::binary);
    if (!is.good()) return false;
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    if (!Json::parseFromStream(builder, is, &root, &errs)) {
        errorstream << "Failed to parse worldmeta.json: " << errs << std::endl;
        return false;
    }
    meta.fromJson(root);
    return true;
}

bool writeWorldMetadata(const std::string &world_path, const WorldMetadata &meta) {
    std::string path = world_path + DIR_DELIM + "worldmeta.json";
    Json::StreamWriterBuilder builder;
    std::string content = Json::writeString(builder, meta.toJson());
    return fs::safeWriteToFile(path, content);
}

bool readWorldIndex(WorldIndex &index) {
    std::string path = porting::path_user + DIR_DELIM + "worlds" + DIR_DELIM + "world_index.json";
    std::ifstream is(path.c_str(), std::ios::binary);
    if (!is.good()) return false;
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    if (!Json::parseFromStream(builder, is, &root, &errs)) {
        errorstream << "Failed to parse world_index.json: " << errs << std::endl;
        return false;
    }
    index.fromJson(root);
    return true;
}

bool writeWorldIndex(const WorldIndex &index) {
    std::string path = porting::path_user + DIR_DELIM + "worlds" + DIR_DELIM + "world_index.json";
    Json::StreamWriterBuilder builder;
    std::string content = Json::writeString(builder, index.toJson());
    return fs::safeWriteToFile(path, content);
}
