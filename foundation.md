# Luanti Unified Engine Architecture & Modding Specification

This document defines the core architecture of the engine's logical separation, world foundation, content extension ecosystem, and the newly implemented Roblox-style modding foundation.

---

# Core Philosophy: Single Responsibility

The engine separates all content, assets, and logic into distinct packages following a strict directory structure. Each directory has exactly **one responsibility**.

| Folder | Responsibility | Details |
| :--- | :--- | :--- |
| **Workspace/** | Definitions | Declarative blueprints and data definitions. No logic or asset files. |
| **Assets/** | Resources | Raw, compiled, or optimized media assets (models, textures, audio, UI layout, etc.). |
| **ClientSideService/** | Client Logic | Client-only Lua scripts responsible for local input, UI, rendering, and audio. |
| **ServerSideService/** | Server Logic | Server-side Lua scripts responsible for gameplay, state management, and authoritative logic. |
| **Storage/** | Runtime Data | Runtime-generated read/write data, local/cloud cache, settings, and database storage. |

Nothing must ever mix these responsibilities.

---

# Part I: Roblox-Style Modding Foundation (Implemented)

The modding foundation introduces a code-driven, highly extensible object-oriented game maker framework inspired directly by Roblox Studio, running atop Luanti's voxel core.

## The Global `Instance` API

The base class is `Instance`, representing any node, container, or logic script in the game hierarchy. Standard instantiation is performed via `Instance.new(className, parent)`.

### Base Class Properties
- `Name`: string (defaults to the ClassName; read/write).
- `ClassName`: string (read-only).
- `Parent`: Instance or `nil` (read/write; moves the instance inside the tree).

### Base Class Methods
- `:GetChildren()`: Returns an array of active child Instances.
- `:GetDescendants()`: Recursively returns a flat array of all descendants.
- `:FindFirstChild(name, recursive)`: Finds the first child with the specified Name.
- `:FindFirstChildOfClass(className, recursive)`: Finds the first child of the specified ClassName.
- `:FindFirstAncestor(name)`: Finds the first ancestor with the specified Name.
- `:Clone()`: Deep-clones the Instance and its children.
- `:Destroy()`: Disconnects all events, destroys associated physical voxel nodes/sounds, clears children, and nullifies parentage.
- `:ClearAllChildren()`: Calls `:Destroy()` on all child instances.

### Universal Attributes System
Allows storing arbitrary custom metadata on any instance:
- `:SetAttribute(name, value)`: Stores any standard Lua type (number, string, boolean, vector).
- `:GetAttribute(name)`: Retrieves the attribute value.
- `:GetAttributes()`: Returns a table containing all attribute key-values.
- `.AttributeChanged`: A Signal that fires `(attribute_name, attribute_value)` when any attribute changes.

### Universal Tags System
Allows grouping any instances together across different services:
- `:AddTag(tag)`: Adds a string tag.
- `:RemoveTag(tag)`: Removes a tag.
- `:HasTag(tag)`: Returns whether the instance has the specified tag.
- `:GetTags()`: Returns an array of all active tags.

### Universal Property Changed Listeners
- `:GetPropertyChangedSignal(property)`: Returns a Signal that fires whenever that specific property is modified. Example: `part:GetPropertyChangedSignal("Position"):Connect(function() ... end)`.

### Roblox-Style Event Signals
A standard `:Connect(callback)` and `:Fire(...)` connection and event-firing framework, returning a connection object with a `:Disconnect()` method.

---

## Implemented Specialized Classes

### 1. `DataModel` (the global `game` root)
- Directly extends `Instance`.
- Exposes `:GetService(name)` which queries or creates organizational service folders on demand.
- Initialized on boot with default services:
  - `game.Workspace`
  - `game.ReplicatedStorage`
  - `game.ServerScriptService`
  - `game.Players`
- Inherits all standard Instance methods (e.g. `game:GetChildren()`).

### 2. `Folder`
- Serving as a standard nested organizational container.

### 3. `Part` or `Block`
- Standard physical voxel brick representing nodes in the active world.
- Properties:
  - `Position`: vector (default `{x=0, y=0, z=0}`)
  - `BlockType`: string (default `"default:stone"`)
  - `Color`: string (default `"#FFFFFF"`)
  - `Size`: vector (default `{x=1, y=1, z=1}`)
  - `Anchored`: boolean (default `true`)
- Behavior:
  - When parented under `game.Workspace` (or any of its descendants), it automatically places a block in the physical Luanti world at `Position` with the chosen `BlockType`.
  - Automatically handles physical positioning updates in the world if `Position` or `BlockType` is modified, and deletes itself (sets to air) if parented to `nil` or destroyed.

### 4. `ViewportFrame`
- Integrates with the cross-platform C++ viewport rendering pipeline.
- Properties:
  - `CameraPosition`: vector (default `{x=0, y=10, z=0}`)
  - `CameraDirection`: vector (default `{x=0, y=-1, z=0}`)
  - `CameraUp`: vector (default `{x=0, y=1, z=0}`)
  - `FOV`: number (default `70`)
  - `Width`/`Height`: integers (default `256`)
  - `FPS`: integer (default `20`)
  - `TargetBlock`: vector or `nil` (node coordinate to apply viewport stream onto)
- Behavior:
  - Automatically updates camera rendering parameters dynamically in C++ on property modifications.
  - Streams the live 3D viewport feed onto the block surface at `TargetBlock` as a dynamic media texture (`"viewport_camera_" .. self.Name`) using C++ `ITextureSource::overrideTexture` bindings.

### 5. `Sound`
- Properties:
  - `SoundName`: string (default `"default:dig_stone"`)
  - `Volume`/`Pitch`: numbers
  - `Looped`: boolean
  - `Playing`: boolean (read/write; toggles sound playing)
- Methods:
  - `:Play()`: Plays the sound. Supports positional 3D sound if parented to a `Part` or `Player`.
  - `:Stop()`: Stops the sound.

### 6. `Script`
- Properties:
  - `Source`: string
- Behavior:
  - Automatically compiles and runs if parented under `game.ServerScriptService` on boot or ancestry change.
  - Executed inside a secure, blacklisted Lua execution sandbox (`core.create_sandbox`) to prevent security exploits.

---

# Part II: Secure Sandbox & Filesystem Rojo Loader

## Smart Blacklist Sandbox (`core.create_sandbox`)
Both Workspace execute_service_script and game Script execution are sandboxed via a smart blacklist to secure the environment while preserving standard Lua and Luanti API scripting power:
- **Blocked direct access to**: `os.execute`, `os.remove`, `os.rename`, `os.exit`, `io` library, `require`, `loadfile`, `dofile`, and `debug`.
- **Blocked `_G` sandbox bypass**: Accessing `_G` redirects to the sandboxed environment proxy, ensuring no hidden access to dangerous system calls.

## Rojo-style Folder Workspace Loader
On server boot, the engine recursively scans `Workspace/Explorer/` in all enabled mods:
- Subdirectories compile into nested `Folder` instances.
- `.lua` files compile into `Script` instances containing the source code.
- `.part`, `.viewport`, `.sound`, `.ini`, and `.json` files are parsed using a built-in INI parser into their respective custom `Instance` types with pre-loaded properties.
- Scripts found under `game.ServerScriptService` automatically execute to boot the mod's game logic.

---

# Part III: Cross-Platform Viewports C++ Pipeline

To make viewports rendering universally available, the rendering pipeline has been ported to compile globally on all desktop (Windows, Linux, macOS) and mobile (Android) systems:

1. **`core.get_dir_listing` C++ Registration**: Ported directory listing utilities to compile in all contexts, enabling server-side workspace discovery.
2. **`htmlview_jni_render_viewports`**: Ported the Irrlicht viewport camera rendering loop from JNI guards to compile globally.
3. **`ITextureSource::overrideTexture`**: Implemented dynamic texture overriding inside the core texture manager, allowing C++ viewports to write live 3D feeds onto custom block materials dynamically.
