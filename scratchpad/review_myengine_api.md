# Review of `myengine` API

The `myengine` API is a new high-level Lua API designed for stability and power, providing a stable alias layer over internal Luanti paths.

## What This System Does
It provides a set of functions (`get`, `set`, `hook`, `modify`, `add`, `remove`) in a global Lua table `myengine`. These functions use an alias system to map stable, human-readable paths to internal Lua/engine paths, ensuring that mods don't break when internals change.

## Key Files
- `src/myengine/myengine.h/cpp`: Implementation of the Lua API functions.
- `src/myengine/aliases.h/cpp`: Implementation of the `AliasMap` class which handles path resolution.
- `worlds/world/myengine/alias_map.txt`: Human-editable file for defining aliases.
- `src/myengine/CMakeLists.txt`: Build configuration for the `myengine` module.

## How The Pieces Connect
1.  **Alias Loading**: In `Server::init()` (in `src/server.cpp`), `g_myengine_aliases.load_from_file()` is called to load mappings from the engine's `myengine/alias_map.txt` and the world's `myengine/alias_map.txt`.
2.  **API Initialization**: In `ServerScripting` constructor (in `src/script/scripting_server.cpp`), `MyEngine::initialize()` is called to register the `myengine` table and its functions in the Lua state.
3.  **Path Resolution**: When a mod calls `myengine.get("some.alias")`, the API calls `g_myengine_aliases.resolve()` to get the real internal path.
4.  **Action**: The API then performs the requested action (e.g., fetching a nested value from Lua globals, or calling a registration function like `core.register_on_generated`).

## Exact Locations
- `src/myengine/`: Core implementation.
- `src/CMakeLists.txt`: Integrated into the main build.
- `src/script/scripting_server.cpp`: Entry point for server-side Lua initialization.
- `src/server.cpp`: Alias map loading during server initialization.

## Gotchas
- **Invalid Aliases**: If an alias is defined but points to a non-existent internal path, a warning is logged and the operation returns `nil` or does nothing, avoiding a crash.
- **Path Syntax**: Paths use dot notation (e.g., `player.physics.gravity`).
- **Hook Mapping**: `myengine.hook("event")` automatically tries to call `core.register_on_<event>`.
- **Mod Loading**: The alias map is loaded before mods are loaded, so aliases are available during mod initialization.
- **Naming Conventions**: In `myengine.add(type, def)`, the `def` table MUST contain a `name` field. Because `myengine` calls register functions from a global context, modders should use the `":"` prefix (e.g., `":modname:itemname"`) to bypass mod name validation or ensure the full name is provided.
- **Supported Types**:
  - `add`: `node`, `entity`, `item`, `craftitem`, `tool`, `alias`.
  - `remove`: `node`, `item`, `craftitem`, `tool`, `entity`, `alias`.
  - `modify`: `node`, `item`, `craftitem`.
