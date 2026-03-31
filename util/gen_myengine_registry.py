import os
import re
import sys

# Configuration
INCLUDE_DIRS = ['.']
EXCLUDE_DIRS = [
    './irrlicht', './sqlite', './lua', './gmp', './json',
    './irrlicht_changes', './util', './script', './benchmark', './unittest'
]
OUTPUT_HEADER = 'myengine_registry.generated.h'
OUTPUT_SOURCE = 'myengine_registry.generated.cpp'

# Whitelist of classes to generate deep access for
WHITELIST = [
    'PlayerSAO', 'UnitSAO', 'Player', 'RemotePlayer',
    'ServerEnvironment', 'Server', 'IGameDef',
    'Inventory', 'InventoryList', 'MapNode', 'ServerActiveObject', 'Environment'
]

# Regex patterns
CLASS_RE = re.compile(r'class\s+([A-Za-z0-9_]+)\s*(?::\s*(?:public|protected|private)\s+[A-Za-z0-9_]+(?:\s*,\s*(?:public|protected|private)\s+[A-Za-z0-9_]+)*)?\s*\{([\s\S]*?)\};')
MEMBER_RE = re.compile(r'^\s*([A-Za-z0-9_:]+(?:\s*\*+)?)\s+([A-Za-z0-9_]+)\s*(?:=\s*[^;]+)?\s*;', re.MULTILINE)

KEYWORDS = {
    'public', 'protected', 'private', 'static', 'friend', 'virtual',
    'using', 'typedef', 'class', 'struct', 'enum', 'template', 'return',
    'explicit', 'inline', 'const', 'constexpr', 'operator'
}

MEMBER_BLACKLIST = {'operator', 'r', 'res', 'size', 'count', 'it', 'i', 'false', 'true', 'nullptr'}

def clean_name(name):
    if name.startswith('m_'):
        name = name[2:]
    return name

def scan_files():
    registry = {}
    for root_dir in INCLUDE_DIRS:
        for root, dirs, files in os.walk(root_dir):
            if any(os.path.join(root_dir, root.lstrip('./')).startswith(exclude) for exclude in EXCLUDE_DIRS):
                continue

            for file in files:
                if file.endswith('.h'):
                    path = os.path.join(root, file)
                    with open(path, 'r', encoding='utf-8', errors='ignore') as f:
                        content = f.read()
                        for class_match in CLASS_RE.finditer(content):
                            class_name = class_match.group(1)
                            class_body = class_match.group(2)
                            if class_name not in WHITELIST: continue

                            members = []
                            for mem_match in MEMBER_RE.finditer(class_body):
                                mem_type = mem_match.group(1).strip()
                                mem_name = mem_match.group(2).strip()
                                if mem_type in KEYWORDS or mem_name in KEYWORDS or mem_name in MEMBER_BLACKLIST: continue
                                members.append({'type': mem_type, 'name': mem_name, 'clean_name': clean_name(mem_name)})

                            if members:
                                if class_name not in registry:
                                    registry[class_name] = {'members': [], 'path': path}
                                registry[class_name]['members'].extend(members)

    return registry

def generate_code(registry):
    header_content = """#pragma once
#include <string>
#include <map>
#include <vector>
#include <functional>

namespace MyEngine {
    struct Member {
        std::string name;
        std::string type;
        std::function<void*(void*)> get_ptr;
    };

    struct ClassMetadata {
        std::string name;
        std::map<std::string, Member> members;
    };

    void registerAll();
    bool dispatch(const std::string &path, void* ptr1 = nullptr, void* ptr2 = nullptr);

    extern std::map<std::string, ClassMetadata> metadata;
}
"""

    source_content = """#include "myengine_registry.generated.h"
#include "log.h"
#include <iostream>
#include <algorithm>

#include "player.h"
#include "server/player_sao.h"
#include "serverenvironment.h"
#include "server.h"
#include "remoteplayer.h"
#include "inventory.h"
#include "mapnode.h"
#include "lua_api/l_myengine.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

extern std::map<std::string, std::vector<int>> hooks_before;
extern std::map<std::string, std::vector<int>> hooks_after;
extern std::map<std::string, int> modifies;
extern std::map<std::string, int> rewrites;

// Template Instantiation Trick
template<typename Tag>
struct Stolen {
  typedef typename Tag::type type;
  static type ptr;
};
template<typename Tag>
typename Stolen<Tag>::type Stolen<Tag>::ptr;

template<typename Tag, typename Tag::type M>
struct Stealer {
  struct Loader {
    Loader() { Stolen<Tag>::ptr = M; }
  };
  static Loader loader;
};
template<typename Tag, typename Tag::type M>
typename Stealer<Tag, M>::Loader Stealer<Tag, M>::loader;

namespace MyEngine {
"""

    stolen_defs = [
        {'class': 'UnitSAO', 'mem': {'name': 'm_hp', 'type': 'u16'}},
        {'class': 'PlayerSAO', 'mem': {'name': 'm_breath', 'type': 'u16'}},
        {'class': 'PlayerSAO', 'mem': {'name': 'm_player_name', 'type': 'std::string'}},
        {'class': 'Player', 'mem': {'name': 'm_name', 'type': 'std::string'}},
        {'class': 'Environment', 'mem': {'name': 'm_time_of_day', 'type': 'u32'}},
        {'class': 'ServerActiveObject', 'mem': {'name': 'm_base_position', 'type': 'v3f'}},
    ]

    for item in stolen_defs:
        source_content += f"struct Tag_{item['class']}_{item['mem']['name']} {{ typedef {item['mem']['type']} {item['class']}::*type; }};\n"

    source_content += "\n}\n"

    for item in stolen_defs:
        source_content += f"template struct Stealer<MyEngine::Tag_{item['class']}_{item['mem']['name']}, &{item['class']}::{item['mem']['name']}>;\n"

    source_content += """
namespace MyEngine {
    std::map<std::string, ClassMetadata> metadata;

    void registerAll() {
        infostream << "MyEngine: Registering auto-generated API" << std::endl;
"""

    for class_name, info in registry.items():
        lua_key = class_name.lower()
        source_content += f'        {{\n            ClassMetadata cm;\n            cm.name = "{class_name}";\n'
        for mem in info['members']:
            get_ptr_val = "nullptr"
            for item in stolen_defs:
                if item['mem']['name'] == mem['name']:
                    if item['class'] == class_name:
                         tag_name = f"Tag_{class_name}_{mem['name']}"
                         get_ptr_val = f"[](void* obj) {{ return (void*)&((( {class_name}*)obj)->*Stolen<{tag_name}>::ptr); }}"
                         break
                    elif class_name == 'PlayerSAO' and item['class'] in ['UnitSAO', 'ServerActiveObject']:
                         tag_name = f"Tag_{item['class']}_{mem['name']}"
                         get_ptr_val = f"[](void* obj) {{ return (void*)&((( {item['class']}*)obj)->*Stolen<{tag_name}>::ptr); }}"
                         break
                    elif class_name == 'RemotePlayer' and item['class'] == 'Player':
                         tag_name = f"Tag_Player_{mem['name']}"
                         get_ptr_val = f"[](void* obj) {{ return (void*)&((( Player*)obj)->*Stolen<{tag_name}>::ptr); }}"
                         break
                    elif class_name == 'ServerEnvironment' and item['class'] == 'Environment':
                         tag_name = f"Tag_Environment_{mem['name']}"
                         get_ptr_val = f"[](void* obj) {{ return (void*)&((( Environment*)obj)->*Stolen<{tag_name}>::ptr); }}"
                         break

            source_content += f'            cm.members["{mem["clean_name"]}"] = Member{{ "{mem["clean_name"]}", "{mem["type"]}", {get_ptr_val} }};\n'
        source_content += f'            metadata["{lua_key}"] = cm;\n        }}\n'

    source_content += """    }

    bool dispatch(const std::string &path, void* ptr1, void* ptr2) {
        lua_State* L = ModApiMyEngine::getLuaState();
        if (!L) return false;

        int rewrite_ref = ModApiMyEngine::getRewrite(path);
        if (rewrite_ref != -1) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, rewrite_ref);
            lua_getfield(L, -1, "replace");
            if (lua_isfunction(L, -1)) {
                if (ptr1) lua_pushlightuserdata(L, ptr1);
                if (ptr2) lua_pushlightuserdata(L, ptr2);
                lua_pcall(L, (ptr1 ? 1 : 0) + (ptr2 ? 1 : 0), 1, 0);
                lua_pop(L, 2);
                return true;
            }
            lua_pop(L, 2);
        }

        auto it_before = hooks_before.find(path);
        if (it_before != hooks_before.end()) {
            for (int ref : it_before->second) {
                lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
                if (ptr1) lua_pushlightuserdata(L, ptr1);
                if (ptr2) lua_pushlightuserdata(L, ptr2);
                lua_pcall(L, (ptr1 ? 1 : 0) + (ptr2 ? 1 : 0), 0, 0);
            }
        }

        auto it_after = hooks_after.find(path);
        if (it_after != hooks_after.end()) {
            for (int ref : it_after->second) {
                lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
                if (ptr1) lua_pushlightuserdata(L, ptr1);
                if (ptr2) lua_pushlightuserdata(L, ptr2);
                lua_pcall(L, (ptr1 ? 1 : 0) + (ptr2 ? 1 : 0), 0, 0);
            }
        }
        return false;
    }
}
"""
    with open(OUTPUT_HEADER, 'w') as f:
        f.write(header_content)
    with open(OUTPUT_SOURCE, 'w') as f:
        f.write(source_content)

if __name__ == "__main__":
    reg = scan_files()
    generate_code(reg)
    print("Done.")
