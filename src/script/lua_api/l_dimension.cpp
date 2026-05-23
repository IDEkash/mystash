#include "l_dimension.h"
#include "lua_api/l_internal.h"
#include "content/subgames.h"
#include "content/world_metadata.h"
#include "porting.h"
#include "filesys.h"
#include "settings.h"
#include "server.h"
#include "common/c_converter.h"

int ModApiDimension::l_create_world(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    const char *gameid = luaL_checkstring(L, 2);
    bool visible = true;
    bool hidden = false;
    std::string seed = "";
    std::string mapgen = "";
    std::vector<std::string> linked_mods;

    if (lua_istable(L, 3)) {
        lua_getfield(L, 3, "visible");
        if (!lua_isnil(L, -1)) visible = lua_toboolean(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, 3, "hidden");
        if (!lua_isnil(L, -1)) hidden = lua_toboolean(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, 3, "seed");
        if (lua_isstring(L, -1)) seed = lua_tostring(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, 3, "mapgen");
        if (lua_isstring(L, -1)) mapgen = lua_tostring(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, 3, "linked_mods");
        if (lua_istable(L, -1)) {
            lua_pushnil(L);
            while (lua_next(L, -2)) {
                if (lua_isstring(L, -1)) linked_mods.push_back(lua_tostring(L, -1));
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);
    }

    std::string path = porting::path_user + DIR_DELIM + "worlds" + DIR_DELIM + sanitizeDirName(name, "world_");

    SubgameSpec gamespec = findSubgame(gameid);
    if (!gamespec.isValid()) {
        lua_pushboolean(L, false);
        lua_pushstring(L, "Invalid gameid");
        return 2;
    }

    try {
        if (!seed.empty()) g_settings->set("fixed_map_seed", seed);
        if (!mapgen.empty()) g_settings->set("mg_name", mapgen);

        loadGameConfAndInitWorld(path, name, gamespec, true);

        WorldMetadata meta;
        readWorldMetadata(path, meta);
        meta.visible = visible;
        meta.hidden = hidden;
        if (!seed.empty()) meta.seed = seed;
        if (!mapgen.empty()) meta.mapgen = mapgen;
        if (!linked_mods.empty()) meta.linked_mods = linked_mods;
        writeWorldMetadata(path, meta);

        lua_pushboolean(L, true);
        lua_pushstring(L, path.c_str());
        return 2;
    } catch (const std::exception &e) {
        lua_pushboolean(L, false);
        lua_pushstring(L, e.what());
        return 2;
    }
}

int ModApiDimension::l_delete_world(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    if (!fs::PathExists(path)) {
        lua_pushboolean(L, false);
        return 1;
    }

    if (!fs::RecursiveDelete(path)) {
        lua_pushboolean(L, false);
        return 1;
    }

    WorldDirIndex index;
    readWorldDirIndex(index);
    for (auto it = index.worlds.begin(); it != index.worlds.end(); ++it) {
        if (it->path == path) {
            index.worlds.erase(it);
            break;
        }
    }
    writeWorldDirIndex(index);

    lua_pushboolean(L, true);
    return 1;
}

int ModApiDimension::l_link_mod(lua_State *L) {
    const char *world_path = luaL_checkstring(L, 1);
    const char *mod_path = luaL_checkstring(L, 2);

    WorldMetadata meta;
    if (!readWorldMetadata(world_path, meta)) {
        lua_pushboolean(L, false);
        return 1;
    }

    meta.linked_mods.push_back(mod_path);
    writeWorldMetadata(world_path, meta);

    lua_pushboolean(L, true);
    return 1;
}

int ModApiDimension::l_enter_world(lua_State *L) {
    const char *world_path = luaL_checkstring(L, 1);
    Server *server = getServer(L);

    // Signal the server to switch world
    server->requestWorldSwitch(world_path);

    lua_pushboolean(L, true);
    return 1;
}

void ModApiDimension::Initialize(lua_State *L, int top) {
    lua_newtable(L);
    int curr_top = lua_gettop(L);

    lua_pushcfunction(L, l_create_world);
    lua_setfield(L, curr_top, "create_world");

    lua_pushcfunction(L, l_delete_world);
    lua_setfield(L, curr_top, "delete_world");

    lua_pushcfunction(L, l_link_mod);
    lua_setfield(L, curr_top, "link_mod");

    lua_pushcfunction(L, l_enter_world);
    lua_setfield(L, curr_top, "enter_world");

    lua_setfield(L, top, "dimension");
}
