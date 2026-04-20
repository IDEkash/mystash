# Fork APIs

This fork adds Android `htmlview` (including headless workers + JSON helpers), extra animator helpers, glTF multi-clip animation support, an independent bone transform API with per-part visibility and persistent smoothing, upgraded animation blending with smoothstep easing and event callbacks, a refined physics and movement model, an accessibility sprint toggle, and an improved Animation & Scaling API with auto-normalization.

## Android: `htmlview` (Lua)

Android-only. On non-Android platforms, calling these functions errors.

Lifecycle note: HTMLViews are owned by the Android activity layout. This fork destroys all active HTMLViews when leaving a world / stopping the server.

### Creating instances

`htmlview.run(id, html)`
- `id`: string
- `html`: string (inline HTML)

`htmlview.run_external(id, root_dir, entry?)`
- `root_dir`: string (directory containing HTML files)
- `entry`: string (default `"index.html"`)
- `root_dir` is sandbox-checked.

### Headless workers (no visible view attached)

`htmlview.run_worker(id, html)`

`htmlview.run_external_worker(id, root_dir, entry?)`

Workers still support `send`, `inject`, `navigate`, and `on_message`, but `display`/`focus` are ignored.

### Showing / positioning

`htmlview.display(id, opts)`
- `opts`:
  - `visible`: boolean (default `true`)
  - `safe_area`: boolean (default `true`)
  - `fullscreen`: boolean (default `false`)
  - `drag_embed` / `draggable`: boolean (default `false`)
  - `border_radius`: number (default `0`)
  - `x`, `y`: number or string `"center"`
  - `width`, `height`: number or string `"fullscreen"`

`htmlview.focus(id)`
- Brings that HTMLView on top when multiple are open.

### Stopping

`htmlview.stop(id)`

### Messaging

`htmlview.send(id, message)`
- `message`: string

`htmlview.send_json(id, value)`
- Encodes a Lua value to JSON and sends it as a string.

`htmlview.on_message(id, cb_or_nil)`
- `cb(message_string)`

`htmlview.on_message_json(id, cb_or_nil)`
- On success: `cb(decoded_table, raw_string)`
- On parse error: `cb(nil, raw_string, error_string)`

`htmlview.on_ready(id, cb_or_nil)`
- `cb()`: Fired once after `onPageFinished`.

`htmlview.pipe(from_id, to_id)`
- Forwards messages from one HTMLView instance to another.

### Navigation / JS

`htmlview.navigate(id, url)`

`htmlview.inject(id, js)`

`htmlview.reload(id)`
- Reloads without destroying the instance.
- For `run_external*`, reloads the current entry.
- For `run*`, reloads the last provided HTML.

### Shared memory IPC

Allows zero-overhead data exchange between HTMLView workers and Lua.

`htmlview.shared_set(key, val)`
- Sets a value in the shared memory store. `val` can be `nil` to remove.

`htmlview.shared_get(key) -> string | nil`
- Retrieves a value from the shared memory store.

Within the HTMLView (Javascript):

`luanti.shared_set(key, val)`

`luanti.shared_get(key) -> string | null`

### Capture

`htmlview.capture(id, opts?)`
- `opts`:
  - `width`: int (0 = default)
  - `height`: int (0 = default)

`htmlview.on_capture(id, cb_or_nil)`
- `cb(png_bytes)` where `png_bytes` is a Lua string containing PNG file bytes.

### Input control

`htmlview.input(id, opts)`
- `opts`:
  - `block_game_input`: boolean (default `false`)
- When enabled and the view is visible, touches outside the HTMLView are swallowed (prevents interacting with the world behind it).

### State query

`htmlview.state(id) -> table | nil`
- Returns `nil` on error, otherwise a table:
  - `exists`: boolean
  - `worker`: boolean
  - `visible`: boolean
  - `ready`: boolean (`true` after `onPageFinished`)

## glTF multi-clip animation (Lua)

glTF/GLB meshes can contain multiple animations. This fork loads each glTF `animations[i]` as a selectable clip.

`ObjectRef:set_animation(frame_range, frame_speed, frame_blend, frame_loop)`
- Legacy form.
- Also clears any previously selected glTF clip.

`ObjectRef:set_animation(opts)`
- New table form (pass only what you need):
  - `clip`: number (0-based) or string (clip name)
  - `range` / `frame_range`: `{x=..., y=...}`
  - `frame`: number (sets `{frame, frame}`)
  - `speed` / `frame_speed`: number
  - `speed_scale`: number (multiplies the chosen speed)
  - `blend` / `frame_blend`: number
  - `loop` / `frame_loop`: boolean
  - `pause` / `paused`: boolean (sets speed to `0`)

`ObjectRef:set_animation_clip(clip, frame_range, frame_speed, frame_blend, frame_loop)`
- Explicit clip selection.

`ObjectRef:get_animation() -> frame_range, frame_speed, frame_blend, frame_loop, clip`

`ObjectRef:get_animation_info() -> table`
- Returns `{ range, speed, blend, loop, clip, duration, progress=nil, bones=nil }`.
- `progress`/`bones` are placeholders (not currently available from the engine).

### Crossfade blending

For skinned meshes (including glTF), `frame_blend` controls crossfade duration (seconds) when switching animations.

## Improved Animation and Scaling API

This API removes the guesswork involved in scaling glTF models and unifies time handling between different model formats (glTF vs B3D/X).

### Scaling API (Normalization)

New properties in `set_properties` to handle model scaling automatically:

- `auto_normalize`: boolean. If `true`, the engine measures the model's actual size and scales it so that 1 unit in the model file equals 1 node in the game world.
- `target_height`: number. Forces the model to a specific height in nodes. Example: `target_height = 1.7` makes a character exactly 1.7 nodes tall, regardless of original export size.
- `model_unit_scale`: vector. An additional multiplier applied after normalization. Useful for making a model wider or thinner without changing its height.

**Example usage:**

```lua
self.object:set_properties({
    visual = "mesh",
    mesh = "character.gltf",
    auto_normalize = true, -- Now 1 unit in file = 1 node in game
    target_height = 1.8,   -- Force character to be exactly 1.8 nodes tall
})
```

### Enhanced Animation API

glTF animations use seconds for their range, while older formats use frames. The `set_animation` API now handles this automatically via a new `time_mode` parameter.

`ObjectRef:set_animation(opts)`
- `time_mode`:
  - `"auto"` (Default): Uses seconds for glTF and frames for other formats.
  - `"seconds"`: Interprets `range` as seconds. For B3D/X models, the engine automatically converts seconds to frames (assuming 24 FPS).
  - `"frames"`: Interprets `range` as frames. For glTF models, the engine automatically converts frames to seconds.

**Example usage:**

```lua
-- Play exactly 2 seconds of animation, regardless of model format
self.object:set_animation({
    range = {x = 0, y = 2.0},
    speed = 1.0,
    time_mode = "seconds"
})
```

### Model Introspection

New methods to query model format details for writing safer, format-agnostic code.

`ObjectRef:get_model_info() -> table`
- Returns:
  - `mesh`: The filename.
  - `format`: `"gltf"` or `"b3d"`.
  - `uses_time`: Boolean, `true` if the model natively uses seconds.
  - `default_speed`: `1.0` for glTF, `15.0` (or similar) for others.

`ObjectRef:get_animation_info() -> table`
- Enhanced to include:
  - `is_gltf`: boolean
  - `unit`: `"seconds"` or `"frames"` (the unit currently used by the object).

### Engine Fixes

- **Fixed "Tiny Model" Bug**: Fixed a bug where setting an animation range with `x` nearly equal to `y` (like a static pose) could cause the model to shrink to near-zero size due to degenerate scaling math.
- **Cleaner glTF Transitions**: Improved the glTF loader to disable interpolation across different animation clips on the internal timeline, ensuring cleaner swaps between animations (like "walk" to "idle").
- **Safety Warnings**: If you use a frame-based speed (like 30) on a glTF model in "auto" mode, the engine prints a warning to help catch mistakes.

## glTF inspection helpers (Lua)

`core.gltf_get_animation_clips(path) -> list`
- Returns `{ {index,name,start,end,duration}, ... }`.

`core.gltf_inspect(path) -> table`
- Returns:
  - `meshes`: `{ {index,name,primitives}, ... }`
  - `bones`: `{ {node,name}, ... }` (joint nodes across skins)
  - `animations`: `{ {index,name,start,end,duration}, ... }`

## Independent Bone Transform API (Lua)

The independent bone transform API is fully implemented and available on all `ObjectRef` (players and entities). Each transform type (position, rotation, scale) is stored and synced independently — calling `set_bone_rotation` will only update the rotation and leave existing position or scale overrides untouched. The API also supports **Per-Part Visibility** for toggling individual bone visibility, and **Persistent Smoothing** for setting default interpolation durations per bone.

### Setting transforms

`ObjectRef:set_bone_position(bone, position, opts?)`

`ObjectRef:set_bone_rotation(bone, rotation, opts?)`

`ObjectRef:set_bone_scale(bone, scale, opts?)`

**Arguments:**

- `position` / `rotation` / `scale`: Can be a table `{x=..., y=..., z=...}` or three separate numbers `x, y, z`. `set_bone_scale` also supports a single number for uniform scaling.
- `opts` (optional): A table containing:
  - `absolute`: boolean (default `false`). If `true`, the override replaces the animation transform entirely. If `false`, it is added on top of the current animation (ideal for head look and other additive overrides).
  - `interpolation`: float (default `0.0`). The time in seconds to smoothly transition to the new transform value.

### Per-Part Visibility

`ObjectRef:set_part_visible(bone, visible)`
- Toggles the visibility of a specific bone/part.
- `bone`: string — The name of the bone.
- `visible`: boolean — `true` to show, `false` to hide.
- On the client, a hidden bone calls `bone->setVisible(false)`, which preserves the bone's actual scale state for when it is shown again (superior to scaling to zero).

**Example:**

```lua
-- Hide the helmet part
player:set_part_visible("Helmet", false)

-- Show it again later
player:set_part_visible("Helmet", true)
```

### Persistent Smoothing

`ObjectRef:set_part_smooth(bone, table)`
- Sets persistent smoothing durations for a bone. When you call `set_bone_position` (or rotation/scale) without an explicit `interpolation` duration, the server uses these stored values as the default.
- `bone`: string — The name of the bone.
- `table`: A table that can contain:
  - `position`: float — Duration in seconds for position changes.
  - `rotation`: float — Duration in seconds for rotation changes.
  - `scale`: float — Duration in seconds for scale changes.

**Example:**

```lua
-- Define a global smoothing for the head
player:set_part_smooth("Head", {rotation = 0.5})

-- This rotation will now automatically take 0.5s to complete
player:set_bone_rotation("Head", {x=0, y=45, z=0})

-- You can still override it for a specific call if needed
player:set_bone_rotation("Head", {x=0, y=-45, z=0}, {interpolation = 0.1})
```

### Bulk override via `set_bone_override`

`ObjectRef:set_bone_override(bone, table)`
- Sets multiple bone properties at once.
- `table` supports the standard transform fields (`position`, `rotation`, `scale`) as well as the new fields:
  - `visible`: boolean — Per-part visibility.
  - `pos_smooth`: float — Persistent smoothing for position.
  - `rot_smooth`: float — Persistent smoothing for rotation.
  - `scale_smooth`: float — Persistent smoothing for scale.

**Example:**

```lua
player:set_bone_override("RightArm", {
    visible = true,
    rot_smooth = 0.2,
    rotation = { vec = {x=90, y=0, z=0} } -- Will use the 0.2s smooth
})
```

### Querying transforms

`ObjectRef:get_bone_position(bone)`

`ObjectRef:get_bone_rotation(bone)`

`ObjectRef:get_bone_scale(bone)`

Each returns a single vector (`{x,y,z}`) representing the current override for that specific transform.

### How it works

- **Independence**: Each transform (position, rotation, scale) is stored and synced independently. Calling `set_bone_rotation` only updates the rotation; existing position or scale overrides are left untouched.
- **Client-side blending**: Overrides are applied in the client-side rendering loop after glTF animation blending has occurred. This ensures that animation clips (like walking) do not reset manual overrides (like head looking) every frame.
- **Synchronization**: Changes made on the server are automatically serialized and sent to all observing clients.
- **Network protocol**: Bone overrides are sent via `AO_CMD_SET_BONE_POSITION`. The `hidden` visibility state is packed into bit 3 of the flags byte, and smoothing values are appended to the packet. This approach maintains backward compatibility with older clients.
- **Euler persistence**: The API stores the exact Euler angles you provide, avoiding gimbal lock or "twisting" issues that often occur when converting back and forth between quaternions and Euler angles.

This implementation enables robust Minecraft-style head movement, procedural animations, modular entity attachments, and per-part visibility control for equipment systems.

## Lua Animator (`core.animator`)

`core.animator.create(object, def)`
- State machine + events + additive layers.

`core.animator.register(animator)`
- Auto-updates each globalstep.

### Global animator event bus

`core.animator.register_on_event(cb)`

`core.animator.unregister_on_event(cb)`
- `cb(animator, object, event_payload)` called for every emitted animation event.

#### Event types

The `event_payload` table contains:

- `event.name`: The type of event.
  - `"jump_start"`: Fired when a humanoid begins a jump.
  - `"land"`: Fired when a humanoid finishes a jump and returns to the ground (idle/walk/run).
  - `"attack_start"`: Fired when an attack animation begins.
  - `"transition"`: A generic event fired whenever the animator switches states.
- `event.from`: The state being left.
- `event.to`: The state being entered.
- `event.blend`: The duration of the blend.
- `event.ctx`: The animator context at the time of the transition.

#### Example: Playing sounds on animation events

```lua
core.animator.register_on_event(function(animator, object, event)
    if event.name == "jump_start" then
        minetest.sound_play("jump", {object = object})
    elseif event.name == "land" then
        minetest.sound_play("land", {object = object})
    end
end)
```

(Note: `core.animator.register_on_event` also works as the standard naming convention.)

### Upgraded animation blending

The core rendering engine (`irr/src/AnimatedMeshSceneNode.cpp`) has been modified to use **smoothstep easing** (ease-in/out) for animation blending, replacing the old linear blending. This makes transitions between animations feel significantly more fluid and natural.

- **Automatic smoothing**: The Lua API (`set_animation` and `set_animation_clip`) now defaults to a `0.1s` blend time if none is specified. Existing mods immediately benefit from smoother transitions without any code changes.
- **Humanoid logic**: The humanoid animator helper tracks state changes more accurately, specifically for jumping and landing, ensuring events fire at the correct moment.

### Humanoid helper

`core.animator.humanoid(object, clips, opts?) -> animator`
- Builds a basic `idle/walk/run/jump/attack` state machine.
- Uses default context `hs` (horizontal speed). For `jump`/`attack`, provide `opts.get_context` that sets `ctx.jumping` / `ctx.attack`.

### Animation end helper

`core.on_animation_end(object, cb)` (alias for `core.animator.on_animation_end`)
- Calls `cb(object)` when the current non-looping animation is expected to end (computed from `ObjectRef:get_animation()`).

## Physics and Movement Model

A refined physics and movement model designed to provide a "snappy" and physical experience, heavily inspired by the feel of high-performance mobile voxel engines.

### How it works

#### 1. Physical gravity and jump

- **True gravity (32.0 nodes/s²)**: The hidden "Factor of 2" engine hack has been removed. Gravity now behaves exactly as defined. At 32 nodes/s², falling is fast and "heavy," matching modern mobile voxel games.
- **One-block jump (9.5 nodes/s)**: This value is specifically tuned to the 32.0 gravity. It allows the player to consistently clear a 1-block height with a small margin, reaching the top level of the adjacent block.

#### 2. Responsive movement

- **Friction (3.0 acceleration)**: By increasing ground acceleration to 3.0, the "ice-skating" feel is removed. The player starts and stops much faster, providing tight, responsive control.
- **Air control (1.25 acceleration)**: Air acceleration is set to 1.25 to provide a balanced amount of mid-air control without feeling floaty.

#### 3. Built-in sprint logic

The engine now monitors your movement input magnitude (joystick push or key pressure):

- **Trigger**: If input magnitude is ≥ 95% (fully pushed).
- **Behavior**: The walking speed is automatically multiplied by 1.3x, resulting in a sprint speed of approximately 5.6 nodes/s.
- **Internal**: This happens in the engine's `applyControl` step, so it works automatically for any input device reaching that threshold.

#### 4. Specialized liquid physics

The engine now differentiates between "Water-like" and "Lava-like" liquids:

- **Standard liquids**: Uses `movement_liquid_sink` (0.4) and `fluidity` (0.2) for a sluggish swim.
- **Lava physics** (`group:lava`): If a node is in the `lava` item group, the engine applies:
  - **Exponential decay**: Velocity is killed rapidly every frame, making it feel like moving through thick liquid.
  - **Constant sink**: A forced downward sink rate of 0.5 blocks per second prevents simply floating on the surface.

#### 5. Advanced movement mechanics

- **Ladder climbing**:
  - **Controlled descend**: You can now descend ladders at the same speed as climbing up by pressing Crouch/Sneak.
  - **Forward boost**: If you press "Forward" while on a ladder, you get an upward boost matching your climb speed, allowing for faster ascending.
  - **Auto-climb**: When `auto_climb` is enabled (via physics override or accessibility setting), simply moving towards a ladder will climb it, and crouching will descend it.
- **Edge-grabbing (sneak)**: Instead of hitting an "invisible wall" at the edge of a block, the new logic reduces velocity by 50% per frame when you hit the sneak limit. This makes the player "slide" into the edge and "catch" it, creating a smoother edge-grab feel.

### Default settings (`minetest.conf`)

| Setting | Default | Description |
|---|---|---|
| `movement_gravity` | `32.0` | Global gravity (nodes/s²) |
| `movement_speed_jump` | `9.5` | Initial upward velocity for jump |
| `movement_speed_walk` | `4.3` | Baseline walking speed |
| `movement_speed_crouch` | `1.3` | Speed while sneaking |
| `movement_acceleration_default` | `3.0` | Ground friction/responsiveness |
| `movement_acceleration_air` | `1.25` | Mid-air maneuverability |
| `movement_liquid_sink` | `0.4` | Downward speed in water |
| `movement_speed_climb` | `3.0` | Vertical ladder speed |

### Lua API (`player:set_physics_override`)

The standard Luanti API now hooks into these refined engine calculations:

```lua
player:set_physics_override({
    speed = 1.0,                -- Multiplies walk/sprint
    jump = 1.0,                 -- Multiplies jump speed
    gravity = 1.0,              -- Multiplies gravity
    speed_climb = 1.0,          -- Multiplies ladder speed
    acceleration_default = 1.0, -- Multiplies ground friction
    auto_climb = false          -- Enables auto-climb and auto-descend on ladders
})
```

### Node groups

- `group:lava`: Adding this to a node definition automatically enables the high-viscosity "Lava Physics."
- `group:disable_jump`: Prevents jumping while standing on or in the node.

## Accessibility Sprint Toggle

An accessibility setting that gates the engine's internal joystick-driven speed boost, allowing players who prefer a consistent walking speed to disable the automatic sprint.

### How it works

- **Logic gate**: The movement physics in `src/client/localplayer.cpp` have been modified. Previously, the engine hard-coded a 1.3x speed multiplier whenever the joystick magnitude reached 0.95 or higher. Now, this multiplier only activates if the new `accessibilitysprintenabled` setting is `true`.
- **Reactive UI**: The toggle in the Accessibility menu under Movement updates the `PlayerSettings` struct in real-time. As soon as you toggle it, the engine immediately changes how it interprets your joystick input without needing a restart.
- **Robustness**: The setting is registered in `src/defaultsettings.cpp`. Even if the setting is missing from a player's `minetest.conf` (e.g., after a settings reset), it defaults to `true` (original behavior) instead of causing a "Setting not found" crash.

### API

- **Lua API**: Access this setting in mods or the main menu using:
  ```lua
  core.settings:get_bool("accessibilitysprintenabled")
  ```
- **C++ API**: Within the engine, it is stored in the `LocalPlayer` settings:
  ```cpp
  player_settings.accessibility_sprint_enabled  // Boolean
  ```

## Fog API (Lua)

Extended volumetric and height-based fog controls.

`core.set_fog(player, params_or_nil)`
- Sets custom fog parameters for a specific player. Pass `nil` to clear.
- `params`:
  - `color`: ColorSpec (default: sky fog color)
  - `fog_start`: number (0..0.99, fraction of view distance)
  - `fog_end`: number (0..1, fraction of view distance)
  - `blend_time`: number (seconds, transition duration)
  - `max_density`: number (0..1, opacity at max height)
  - `max_density_height`: number (node-space height for max density)
  - `zero_density_height`: number (node-space height where fog disappears)
  - `uniform`: boolean (if true, ignores height density)
  - `direction`: v3f (up vector for height calculation, default `{x=0,y=1,z=0}`)
  - `turbulence`: number (0..1, noise factor)
  - `speed_density_scale`: number (multiplier for density based on player speed)
  - `layers`: list of table (up to 4 extra fog layers):
    - `color`, `max_density`, `max_density_height`, `zero_density_height`, `uniform`, `direction`
  - `color_transition`: table (dynamic color animation):
    - `speed`: number (animation speed)
    - Array of keyframes or `keyframes` field:
      - `{ time=number(0..1), color=ColorSpec }`

`core.set_fog_boundary(player, params_or_nil)`
- Defines a localized fog zone.
- `params`:
  - `pos`: v3f (center of the zone)
  - `radius`: number (node-space size)
  - `shape`: string (`"sphere"`, `"box"`, `"cylinder"`)
  - `fog`: table (FogParams structure as defined above)
  - `sound`: table (optional ambient sound inside zone):
    - `name`: string
    - `gain`: number
    - `fade_in`: number (seconds)

`core.register_biome_atmosphere(biome_id, params)`
- Registers fog and/or boundary parameters for a specific biome.
- `params`:
  - `fog`: table (FogParams)
  - `boundary`: table (FogBoundaryParams)

---
- **More Soon!**
- Latest Update: April, 27, 2026
