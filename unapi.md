# Universal Engine Extension Architecture (`unapi`)

## 1. Overview & Architectural Philosophy

Luanti's traditional modding API exposes functionality through feature-specific C++ Lua bindings. When developers need new engine-level capabilities—such as secondary rendering cameras, offscreen render targets, multi-world rendering, or portal rendering—they are traditionally forced to modify C++ source code, write dedicated C++ APIs, and register feature-specific Lua bindings. This cycle inflates core engine complexity and locks features into single-use implementations.

The **Universal Engine Extension Architecture (`unapi`)** replaces this paradigm with a single, general-purpose extension boundary. Instead of predicting every feature developers might want, `unapi` exposes the major engine foundations—**Render**, **Gameplay**, and **Asset**—through unified, discoverable, and composable extension primitives.

```
                         LUANTI ENGINE
                              │
          ┌───────────────────┼───────────────────┐
          │                   │                   │
       RENDER              GAMEPLAY             ASSETS
          │                   │                   │
          └───────────────────┼───────────────────┘
                              │
                    UNIVERSAL EXTENSION
                           LAYER (`unapi`)
                              │
             ┌────────────────┼────────────────┐
             │                │                │
          inspect          modify           create
             │                │                │
          values           logic            objects
          functions        hooks            resources
          events           override         references
             │                │                │
             └────────────────┼────────────────┘
                              │
                         EXTENSIONS
                              │
             ┌────────────────┼─────────────────┐
             ▼                ▼                 ▼
       Multi-camera       Portals          New systems
       Render-to-texture  Mirrors           New gameplay
       Replay cameras     Dimensions        New animation
       CCTV               Custom renderer    etc.
```

### Key Design Principles

1. **Primitive-Based Expressiveness**: Engine features are built externally by combining foundational primitives (e.g. `render.camera`, `render.pass`, `render.target`, `game.world`) rather than creating feature-specific C++ APIs.
2. **Dynamic Discovery & Reflection**: Developers can query object types, properties, methods, permissions, and stability at runtime without hardcoded Lua bindings.
3. **Capability-Based Security**: Extensions run with isolated sandboxed storage (`extension_data/<extension-id>/`) and explicit permissions (`render.access`, `world.modify`, `filesystem.storage`, `network.http`).
4. **Hot-Path Efficiency**: Hot execution loops use handles and cached references (`unapi.resolve()`) to eliminate string reflection overhead per frame.
5. **Non-Destructive Coexistence**: `unapi` operates alongside existing Luanti Lua APIs, ensuring 100% backwards compatibility for existing mods.

---

## 2. Core API Specification (`unapi` Namespace)

The global `unapi` table (also exposed via `core.unapi`) provides the core operations of the Universal Engine Extension API.

### 2.1 Introspection & Reflection

#### `unapi.inspect(target)`
Inspects a registered extension type or an object instance.

* **Parameters**:
  * `target` (`string` or `UnapiObject`): Type name (e.g. `"render.camera"`) or object instance handle.
* **Returns**: `table` containing reflection metadata:
  * `type_name` (`string`): Registered type identifier.
  * `foundation` (`string`): Engine foundation (`"render"`, `"game"`, or `"asset"`).
  * `description` (`string`): Human-readable type documentation.
  * `instantiable` (`boolean`): Whether instances can be created via `unapi.create()`.
  * `stability` (`string`): API stability (`"stable"`, `"experimental"`, `"internal"`, `"deprecated"`).
  * `properties` (`table`): Map of property descriptors (`type`, `access`, `permission`, `description`).
  * `functions` (`table`): Map of function descriptors (`return_type`, `thread_safe`, `permission`, `description`).
  * `events` (`table`): Map of event descriptors.
  * `handle_id` (`integer`, instance only): Unique numeric instance handle.
  * `child_keys` (`array`, instance only): List of child object keys.

```lua
local info = unapi.inspect("render.camera")
print("Foundation: " .. info.foundation)
for prop_name, prop in pairs(info.properties) do
    print("Property: " .. prop_name .. " (" .. prop.type .. ")")
end
```

---

### 2.2 Object Creation & Instance Lifecycle

#### `unapi.create(type_name, [extension_id])`
Creates a new independent instance of a registered extension type.

* **Parameters**:
  * `type_name` (`string`): Type identifier (e.g., `"render.camera"`, `"render.target"`, `"render.pass"`, `"game.world"`, `"asset.material"`).
  * `extension_id` (`string`, optional): Extension identifier for permission auditing. Defaults to `"default"`.
* **Returns**: `UnapiObject` wrapper instance or `nil` if unsupported.

```lua
local camera = unapi.create("render.camera")
local target = unapi.create("render.target")
local pass = unapi.create("render.pass")
```

#### Object Instance Methods & Metamethods
Instances support direct property getters/setters, method invocation, and child object traversal:

* `obj.property_name`: Reads object property value.
* `obj.property_name = value`: Writes object property value.
* `obj:get_property(name)`: Explicit property getter.
* `obj:set_property(name, value)`: Explicit property setter.
* `obj:invoke_method(method_name, ...)`: Method invocation.
* `obj:get_handle()`: Returns unique 64-bit integer handle ID.
* `obj:get_info()`: Returns instance reflection metadata.
* `obj1 == obj2`: Standard equality comparison between handles.
* `tostring(obj)`: Formats instance representation (`"UnapiObject(render.camera:1001)"`).

---

### 2.3 Handles & Hot-Path Resolution

#### `unapi.resolve(target)`
Resolves numeric handle IDs or type descriptors into cached object handles for hot path execution.

* **Parameters**:
  * `target` (`integer` or `string`): Numeric handle ID or type identifier.
* **Returns**: `UnapiObject` or type reflection table.

```lua
local handle = camera:get_handle()
local cached_camera = unapi.resolve(handle)
```

---

### 2.4 Execution Hooks Subsystem

Hooks allow extensions to intercept, observe, modify arguments, or override execution at deterministic engine execution points.

#### `unapi.hook(hook_name, callback, [options])`
Registers an execution hook.

* **Parameters**:
  * `hook_name` (`string`): Engine hook point (e.g., `"render.pass.execute"`, `"entity.render"`, `"physics.step"`, `"world.update"`).
  * `callback` (`function`): Function `cb(arg1, arg2, ...)` returning `(modified_result, override_boolean)`.
  * `options` (`table`, optional):
    * `timing` (`string`): `"before"` (default), `"override"`, or `"after"`.
    * `extension_id` (`string`): Registering extension ID.
* **Returns**: `integer` numeric hook handle ID.

#### `unapi.unhook(hook_id)`
Unregisters an execution hook.

```lua
local hook_id = unapi.hook("render.pass.execute", function(pass_id, camera, target)
    print("Intercepted render pass execution for camera handle: " .. camera:get_handle())
    -- Return custom result and set override flag to true if replacing execution
    return true, false
end, { timing = "before", extension_id = "my_addon" })

-- Unregister when done
unapi.unhook(hook_id)
```

---

### 2.5 Sandboxed Persistent Storage

Extensions receive an isolated, secure filesystem directory rooted at `extension_data/<extension-id>/`. Extensions cannot access paths outside their designated storage sandbox.

#### `unapi.storage.get(extension_id)`
Returns an `ExtensionStorage` handle for the given extension namespace.

* **Storage Methods**:
  * `storage:exists(relative_path)`: Returns `boolean`.
  * `storage:create_dir(relative_path)`: Creates directory structure.
  * `storage:read(relative_path)`: Reads file content as string.
  * `storage:write(relative_path, content)`: Safely writes file content.
  * `storage:append(relative_path, content)`: Appends content to file.
  * `storage:delete(relative_path)`: Removes file or empty directory.
  * `storage:rename(old_relative_path, new_relative_path)`: Renames/moves file.
  * `storage:list(relative_path)`: Returns array of string file names.

```lua
local storage = unapi.storage.get("my_extension")
storage:write("config/settings.json", '{"fov": 90, "shadows": true}')
if storage:exists("config/settings.json") then
    local data = storage:read("config/settings.json")
end
```

---

### 2.6 External Network Services

Supplements local GPU capabilities with secure HTTP/HTTPS service requests.

#### `unapi.network.http_fetch(url, [options])`
Performs a synchronous or asynchronous HTTP request subject to the `network.http` capability permission.

* **Parameters**:
  * `url` (`string`): Destination URL.
  * `options` (`table`, optional):
    * `method` (`string`): `"GET"`, `"POST"`, `"PUT"`, `"DELETE"`, `"HEAD"`.
    * `headers` (`table`): Key-value request headers map.
    * `data` (`string`): Request body payload.
    * `timeout` (`integer`): Timeout in milliseconds.
    * `extension_id` (`string`): Extension identifier.
* **Returns**: `table` containing `{ status = integer, body = string, succeeded = boolean, error = string }`.

---

## 3. Foundation Specifications

### 3.1 Render Foundation (`render`)

| Type Name | Foundation | Description | Key Properties / Methods |
| :--- | :--- | :--- | :--- |
| `render.camera` | `render` | Independent 3D camera | `position`, `rotation`, `fov`, `enabled`, `world`, `set_transform()`, `look_at()`, `render()` |
| `render.target` | `render` | Framebuffer offscreen target | `width`, `height`, `texture`, `resize(w, h)`, `get_texture()`, `bind()`, `unbind()` |
| `render.pass` | `render` | Configurable render pass step | `camera`, `target`, `world`, `enabled`, `recursion_depth`, `max_recursion_depth`, `execute()` |
| `render.texture` | `render` | GPU Texture resource | `name`, `width`, `height`, `format`, `get_reference()` |
| `render.shader` | `render` | Custom pipeline shader | `name`, `vertex_source`, `fragment_source`, `bind()` |
| `render.mesh` | `render` | Custom geometry | `vertex_count`, `index_count`, `set_vertices()`, `set_indices()` |
| `render.visual_state` | `render` | Lighting / sky / fog override | `sky_color`, `fog_distance` |

### 3.2 Gameplay Foundation (`game`)

| Type Name | Foundation | Description | Key Properties / Methods |
| :--- | :--- | :--- | :--- |
| `game.world` | `game` | Game world or dimension instance | `name`, `instance_id`, `time_of_day`, `gravity`, `spawn_entity()`, `update()`, `step_physics()` |
| `game.entity` | `game` | Active entity instance | `id`, `position`, `rotation`, `velocity`, `move()`, `destroy()` |
| `game.player` | `game` | Player instance | `name`, `position`, `rotation`, `camera` |
| `game.physics` | `game` | Physics state manager | `step()` |

### 3.3 Asset Foundation (`asset`)

| Type Name | Foundation | Description | Key Properties / Methods |
| :--- | :--- | :--- | :--- |
| `asset.model` | `asset` | 3D Asset model resource | `path`, `skeleton`, `get_skeleton()` |
| `asset.skeleton` | `asset` | Skeleton bone structure | `bone_count`, `get_bone_transform()`, `set_bone_transform()` |
| `asset.animation` | `asset` | Animation track | `name`, `speed`, `loop`, `play()`, `stop()` |
| `asset.material` | `asset` | Material & texture binding | `name`, `texture`, `shader`, `set_texture()` |
| `asset.texture` | `asset` | Asset image resource | `path` |

---

## 4. Capability Permissions & Security Model

Extensions operate under capability-based permissions to prevent malicious or unsafe memory/system operations:

* `render.access`: Basic rendering object access.
* `render.create_resources`: Allocation of offscreen textures and render targets.
* `filesystem.storage`: Access to sandboxed storage (`extension_data/<extension-id>/`).
* `network.http`: Permission to issue external HTTP/HTTPS network requests.
* `network.websocket`: Permission to establish WebSocket connections.
* `world.modify`: Permission to create dimension instances and modify entity/world state.
* `physics.modify`: Permission to override physics stepping.

---

## 5. Validation Implementations & Proofs of Concept

### 5.1 Proof of Concept A: Multi-Camera Addon

An extension creates and operates a secondary independent camera, binds it to an offscreen target, and renders the scene.

```lua
-- Multi-Camera Extension Addon
local sec_camera = unapi.create("render.camera")
sec_camera.position = {x = 50, y = 20, z = -100}
sec_camera.rotation = {x = 0, y = 90, z = 0}
sec_camera.fov = 80.0

local sec_target = unapi.create("render.target")
sec_target:resize(1280, 720)

local sec_pass = unapi.create("render.pass")
sec_pass.camera = sec_camera
sec_pass.target = sec_target

-- Execute render pass for secondary camera
sec_pass:execute()

local resulting_texture = sec_target.texture
print("Captured secondary camera view to GPU texture: " .. resulting_texture.name)
```

---

### 5.2 Proof of Concept B: Render-To-Texture Pipeline

Captures scene rendering into an offscreen render target texture and applies that texture to a 3D surface material.

```lua
-- Render-To-Texture Addon
local rtt_camera = unapi.create("render.camera")
local rtt_target = unapi.create("render.target")
local rtt_pass = unapi.create("render.pass")
local rtt_material = unapi.create("asset.material")

rtt_target:resize(1024, 1024)
rtt_pass.camera = rtt_camera
rtt_pass.target = rtt_target

-- Render pass writes output to rtt_target.texture
rtt_pass:execute()

-- Assign dynamic render-target texture to visual material
rtt_material.texture = rtt_target.texture
print("Rendered scene to texture and assigned to material surface successfully.")
```

---

### 5.3 Proof of Concept C: Immersive Portals Extension

An extension constructing a complete Immersive Portals system **entirely in Lua** without dedicated engine "Portal" C++ code. The portal calculates relative destination transforms, operates a secondary camera targeting a destination world instance, renders the view to a render target, and projects the texture onto the portal surface.

```lua
-- Immersive Portals Extension (Built on General-Purpose Extension Primitives)
local ImmersivePortal = {
    entrance_pos = {x = 0, y = 2, z = 0},
    exit_pos = {x = 5000, y = 100, z = 5000},
    exit_yaw_offset = 180.0,
    max_recursion = 3
}

function ImmersivePortal:new(destination_world)
    local instance = setmetatable({}, { __index = ImmersivePortal })
    instance.destination_world = destination_world

    -- Universal Extension Objects
    instance.camera = unapi.create("render.camera")
    instance.target = unapi.create("render.target")
    instance.pass = unapi.create("render.pass")
    instance.material = unapi.create("asset.material")

    -- Composition setup
    instance.target:resize(1024, 1024)
    instance.camera.world = instance.destination_world
    instance.pass.camera = instance.camera
    instance.pass.target = instance.target
    instance.pass.world = instance.destination_world
    instance.pass.max_recursion_depth = instance.max_recursion
    instance.material.texture = instance.target.texture

    return instance
end

function ImmersivePortal:updateTransform(player_cam_pos, player_cam_rot)
    -- 1. Calculate relative offset from entrance portal
    local rel_x = player_cam_pos.x - self.entrance_pos.x
    local rel_y = player_cam_pos.y - self.entrance_pos.y
    local rel_z = player_cam_pos.z - self.entrance_pos.z

    -- 2. Transform relative offset to exit portal coordinate space
    local dest_x = self.exit_pos.x + rel_x
    local dest_y = self.exit_pos.y + rel_y
    local dest_z = self.exit_pos.z + rel_z
    local dest_yaw = player_cam_rot.y + self.exit_yaw_offset

    -- 3. Update secondary camera transform
    self.camera.position = {x = dest_x, y = dest_y, z = dest_z}
    self.camera.rotation = {x = player_cam_rot.x, y = dest_yaw, z = player_cam_rot.z}
end

function ImmersivePortal:renderPortalSurface()
    -- Render pass execution automatically checks recursion depth limit
    return self.pass:execute()
end

-- Usage Example:
local nether_world = unapi.create("game.world")
nether_world.name = "nether"
nether_world.instance_id = 1

local portal = ImmersivePortal:new(nether_world)

-- On player frame update:
portal:updateTransform({x = 0, y = 2, z = -5}, {x = 0, y = 0, z = 0})
local success = portal:renderPortalSurface()

if success then
    print("Portal view rendered successfully to destination world!")
else
    print("Portal render skipped or recursion limit reached.")
end
```

---

## 6. Summary of Architectural Success Criteria

The Universal Engine Extension Architecture successfully fulfills all design requirements:

1. **Feature Decoupling**: Complex engine-level features (Multi-Camera, Render-to-Texture, Immersive Portals) are implemented entirely in extensions without modifying core engine C++ for each feature.
2. **Dynamic Discovery**: Reflection system (`unapi.inspect`) reveals properties, methods, events, and permissions dynamically.
3. **Cross-Foundation References**: Objects seamlessly reference each other across Render, Gameplay, and Asset foundations (`Camera -> RenderPass -> World -> RenderTarget -> Texture -> Material`).
4. **Deterministic Execution Hooks**: Fine-grained execution control (`unapi.hook`) with recursion limit protection prevents infinite loops.
5. **Security & Sandboxing**: Persistent sandboxed storage (`extension_data/<id>/`) and capability-based permissions ensure safety on Android devices.
6. **Zero Regression**: Existing Luanti mods run unchanged with full backwards compatibility.
