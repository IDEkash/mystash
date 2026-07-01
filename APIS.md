# Fork APIs

This fork adds Android `htmlview` (including headless workers + JSON helpers), extra animator helpers, glTF multi-clip animation support, an independent bone transform API with per-part visibility and persistent smoothing, upgraded animation blending with smoothstep easing and event callbacks, a refined physics and movement model, an accessibility sprint toggle, an improved Animation & Scaling API with auto-normalization, dedicated look-direction synchronisation, and a server-side world creation API.

## Player Synchronisation Improvements

This fork introduces a dedicated network packet for updating look direction independently of position.

### Improved `set_look_vertical` / `set_look_horizontal`

Previously, calling these methods triggered a full "teleport" packet (`TOCLIENT_MOVE_PLAYER`) that reset the player's position on the client. This effectively "froze" the player if called frequently (e.g., for smooth camera animations).

- **New Behavior**: On supported clients (protocol version >= 52), these methods now only sync the look direction. The player can continue moving freely while their camera orientation is controlled by the server.
- **Backward Compatibility**: Automatically falls back to the old behavior (teleport) for older clients to ensure they still see the orientation change.

---

## Camera API (Lua)

Server-side `ObjectRef` methods for controlling the player's camera.

### Camera State

`ObjectRef:set_camera(table)`
- `table` fields:
  - `mode`: string (`"firstperson"`, `"thirdpersonback"`, `"thirdpersonfront"`)
  - `free_look`: boolean (default `false`). If `true`, server-forced orientation updates (via `set_look_vertical` or recoil) are applied additively to the player's current orientation rather than overriding it.
  - `smooth`: boolean (default `false`). If `true`, orientation changes are smoothed on the client (0.05s default window) even if cinematic mode is off.
  - `tilt`: number (default `0`). Sets the camera roll (tilt) in degrees.
  - `anti_tilt_controller`: boolean (default `false`). If `true`, the player's look and movement controls remain fixed to the screen even when the camera is tilted. If `false`, controls rotate with the camera (tilting 90 degrees right means pushing Forward moves you Right in the world).
  - `fov`: number. Sets the Field of View. Set to `0` to reset to client default.
  - `fov_is_multiplier`: boolean (default `false`). If `true`, the `fov` value is treated as a multiplier for the player's base FOV setting.
  - `fov_transition`: number (default `0.0`). Duration in seconds for a smooth FOV transition.

`ObjectRef:get_camera() -> table`
- Returns a table containing all the fields listed above.

### Field of View

`ObjectRef:set_fov(degrees, is_multiplier?, transition_time?)`
- Legacy/explicit form of FOV control.
- `degrees`: number. `0` to reset.
- `is_multiplier`: boolean (default `false`).
- `transition_time`: number (default `0`).

`ObjectRef:get_fov() -> table`
- Returns `{fov, is_multiplier, transition_time}`.

---

## Android: `htmlview` (Lua)

Android-only. On non-Android platforms, calling these functions errors.

Lifecycle note: HTMLViews are owned by the Android activity layout. This fork destroys all active HTMLViews when leaving a world / stopping the server.

### Support Query

`htmlview.is_supported() -> boolean`
- Returns `true` if the current platform supports HTMLViews (currently Android-only).
- On unsupported platforms, `htmlview` functions are still available as dummy functions that log a single warning to the console, preventing mod crashes.

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
  - `border_radius`: number (default `0`, negative values are clamped to `0`)
  - `x`, `y`: number (pixel position, default `0`) or string `"center"`
  - `width`, `height`: number (pixels, default `1`) or string `"fullscreen"` (setting either to `"fullscreen"` is equivalent to passing `fullscreen = true`)

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
- **Note:** `val` must be a string or `nil`. Non-string values must be serialised before calling.

`htmlview.shared_get(key) -> string | nil`
- Retrieves a value from the shared memory store.
- Returns `nil` both when the key does not exist **and** when the stored value is an empty string — the two cases are indistinguishable from Lua.

Within the HTMLView (Javascript):

`luanti.shared_set(key, val)`

`luanti.shared_get(key) -> string | null`

### Capture

`htmlview.capture(id, opts?)`
- `opts`:
  - `width`: int (default `0` = use view's natural size)
  - `height`: int (default `0` = use view's natural size)
  - Negative values are clamped to `0`.

`htmlview.on_capture(id, cb_or_nil)`
- `cb(png_bytes)` where `png_bytes` is a Lua string containing PNG file bytes.

### Input control

`htmlview.input(id, opts)`
- `opts`:
  - `block_game_input`: boolean (default `false`)
- When enabled and the view is visible, touches outside the HTMLView are swallowed (prevents interacting with the world behind it).

### State query

`htmlview.state(id) -> table | nil`
- Returns `nil` on error, on non-Android platforms, or if the JSON from the native side cannot be parsed. Otherwise a table:
  - `exists`: boolean
  - `worker`: boolean
  - `visible`: boolean
  - `ready`: boolean (`true` after `onPageFinished`)

### Viewports (Secondary Cameras)

Allows rendering a secondary camera view into an HTML element.

`htmlview.set_viewport(id, name, params)`
- `id`: string (HTMLView instance ID)
- `name`: string (Viewport name, used in the URL scheme)
- `params`: table (or `nil` to remove the viewport)
  - `pos`: vector (world position)
  - `dir`: vector (look direction)
  - `up`: vector (up direction, default `{x=0, y=1, z=0}`)
  - `fov`: number (Field of View in degrees, default `70`)
  - `tilt`: number (camera roll in degrees, default `0`)
  - `width`: number (render resolution width, default `256`)
  - `height`: number (render resolution height, default `256`)
  - `fps`: number (refresh rate, default `20`. Set to `0` for manual dirty-only updates)
  - `format`: string (`"jpeg"` or `"png"`, default `"jpeg"`)
  - `quality`: number (`1..100`, only for `"jpeg"`, default `70`)
  - `smooth_position`: boolean (default `false`)
  - `smooth_rotation`: boolean (default `false`)
  - `position_smoothing`: number (default `0.15`)
  - `rotation_smoothing`: number (default `0.10`)
  - `update_mode`: string (`"continuous"`, `"manual"`, `"on_change"`, default `"continuous"`)

`htmlview.get_viewport(id, name) -> table | nil`
- Returns the current parameters of a viewport.

`htmlview.update_viewport(id, name, params)`
- Updates existing viewport parameters. Only fields present in `params` are updated.

`htmlview.remove_viewport(id, name)`
- Removes a viewport.

`htmlview.get_viewport_frame(id, name) -> string | nil`
- Returns the current frame of the viewport as a raw binary string (JPEG or PNG).

`htmlview.get_viewport_list(id) -> table`
- Returns a list of all viewport names for a given HTMLView instance.

### Anchors (3D Billboards & Planes)

Allows attaching WebView content or Viewports to world positions or objects.

`htmlview.set_anchor(id, [name], params)`
- `id`: string
- `name`: string (optional, defaults to `"main"`)
- `params`: table (or `nil` to remove)
  - `type`: string (`"2d"`, `"3d"`, or `"plane"`, default `"2d"`)
  - `target`: vector (world position) or ObjectRef
  - `offset`: vector (offset from target)
  - `size`: vector2 (width and height for `"3d"`/`"plane"`)
  - `rotation`: vector (Euler angles for `"plane"`)
  - `viewport`: string (optional, name of a viewport to display instead of the WebView)

`htmlview.remove_anchor(id, name)`

### Viewframeports (Combined Viewport & Billboard)

A convenience API that creates both a viewport and a 3D anchor that displays it.

`htmlview.set_viewframeport(id, name, params)`
- `id`: string
- `name`: string
- `params`: table (combines viewport and anchor parameters)

`htmlview.get_viewframeport(id, name) -> table | nil`
- Returns the current parameters of a viewframeport.
  - `pos`: vector (viewport/camera position OR anchor position if `target` is not set)
  - `dir`: vector (look direction)
  - `fov`: number
  - `width`, `height`: number (resolution)
  - `fps`: number
  - `size`: vector2 (billboard size in world)
  - `anchor_type`: string (`"3d"` or `"plane"`, default `"3d"`)
  - `offset`: vector (anchor offset from `pos` or `target`)
  - `target`: vector or ObjectRef (anchor target)

`htmlview.remove_viewframeport(id, name)`

**Usage in HTML (Javascript):**

To display the viewport feed, use the custom `luanti-viewport://` scheme. Since it's a live feed, you'll typically want to refresh it periodically.

```html
<img id="my_camera" src="luanti-viewport://main">

<script>
    const img = document.getElementById('my_camera');
    setInterval(() => {
        // Appending a timestamp prevents caching issues in the WebView
        img.src = "luanti-viewport://main?t=" + Date.now();
    }, 50); // ~20 FPS
</script>
```

---

## glTF multi-clip animation (Lua)

glTF/GLB meshes can contain multiple animations. This fork loads each glTF `animations[i]` as a selectable clip.

`ObjectRef:set_animation(frame_range, frame_speed, frame_blend, frame_loop)`
- Legacy positional form.
- Also clears any previously selected glTF clip.
- Default `frame_blend` is `0.1` (smoothstep blend).

`ObjectRef:set_animation(opts)`
- New table form (pass only what you need):
  - `clip`: number (0-based index) or string (clip name)
  - `range` / `frame_range`: `{x=..., y=...}` (defaults to the entire clip duration if `clip` is specified, otherwise `{1, 1}`)
  - `frame`: number (sets `{frame, frame}`)
  - `speed` / `frame_speed`: number
  - `speed_scale`: number (multiplies the chosen speed)
  - `blend` / `frame_blend`: number (default `0.1`)
  - `loop` / `frame_loop`: boolean
  - `pause` / `paused`: boolean (sets speed to `0`)
  - `time_mode`: `"auto"` | `"seconds"` | `"frames"` (see Enhanced Animation API)

`ObjectRef:set_animation_clip(clip, frame_range, frame_speed, frame_blend, frame_loop)`
- Explicit clip selection. `clip` can be a number (0-based index) or a string (clip name).

`ObjectRef:get_animation() -> frame_range, frame_speed, frame_blend, frame_loop, clip`
- Returns five values. `clip` is a number (index), string (name), or `nil` if no clip is selected.

`ObjectRef:get_animation_info() -> table`
- Returns:
  - `range`: `{x, y}`
  - `speed`: number
  - `blend`: number
  - `loop`: boolean
  - `clip`: number, string, or `nil`
  - `duration`: number (seconds; `0` if speed is zero or range is degenerate)
  - `progress`: `nil` (placeholder, not currently available from the engine)
  - `bones`: `nil` (placeholder, not currently available from the engine)
  - `is_gltf`: boolean
  - `unit`: `"seconds"` or `"frames"` (the unit the object is currently using)

### Crossfade blending

For skinned meshes (including glTF), `frame_blend` controls crossfade duration (seconds) when switching animations.

---

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
  - `"auto"` (Default): Uses seconds for glTF and frames for other formats. Prints a warning if speed > 5.0 is used with a glTF model (likely a legacy frame-speed mistake).
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
  - `mesh`: string — The filename from object properties.
  - `format`: `"gltf"` or `"b3d"` — Determined by file extension (`.gltf`/`.glb` → `"gltf"`, anything else → `"b3d"`).
  - `uses_time`: boolean — `true` if the model natively uses seconds (i.e. glTF).
  - `default_speed`: `1.0` for glTF, `15.0` for others.

`ObjectRef:get_animation_info() -> table`
- Enhanced to include (in addition to all fields listed above):
  - `is_gltf`: boolean
  - `unit`: `"seconds"` or `"frames"` (the unit currently used by the object)

### Engine Fixes

- **Fixed "Tiny Model" Bug**: Fixed a bug where setting an animation range with `x` nearly equal to `y` (like a static pose) could cause the model to shrink to near-zero size due to degenerate scaling math.
- **Cleaner glTF Transitions**: Improved the glTF loader to disable interpolation across different animation clips on the internal timeline, ensuring cleaner swaps between animations (like "walk" to "idle").
- **Safety Warnings**: If you use a frame-based speed (like 30) on a glTF model in `"auto"` mode (speed > 5.0), the engine prints a warning to help catch mistakes.

---

## glTF inspection helpers (Lua)

`core.gltf_get_animation_clips(path) -> list | nil, error_string`
- On success, returns an array: `{ {index, name, start, end, duration}, ... }`
  - `start` is always `0.0`. `end` and `duration` are equal and reflect the measured clip length.
- On parse error, returns `nil, error_string`.

`core.gltf_inspect(path) -> table | nil, error_string`
- On success, returns:
  - `meshes`: `{ {index, name, primitives}, ... }`
  - `bones`: `{ {node, name}, ... }` — joint nodes across all skins. **Order is non-deterministic** (built from a hash set).
  - `animations`: `{ {index, name, start, end, duration}, ... }` — same format as `gltf_get_animation_clips`. `start` is always `0.0`.
- On parse error, returns `nil, error_string`.

Both functions are subject to the engine's secure path check.

---

## Independent Bone Transform API (Lua)

The independent bone transform API is fully implemented and available on all `ObjectRef` (players and entities). Each transform type (position, rotation, scale) is stored and synced independently — calling `set_bone_rotation` will only update the rotation and leave existing position or scale overrides untouched. The API also supports **Per-Part Visibility** for toggling individual bone visibility, and **Persistent Smoothing** for setting default interpolation durations per bone.

### Setting transforms

`ObjectRef:set_bone_position(bone, position, opts?)`

`ObjectRef:set_bone_rotation(bone, rotation, opts?)`

`ObjectRef:set_bone_scale(bone, scale, opts?)`

**Arguments:**

- `position` / `rotation` / `scale`: Can be a table `{x=..., y=..., z=...}` or three separate numbers `x, y, z`. `set_bone_scale` also supports a single number for uniform scaling.
- **Rotation units:** `set_bone_rotation` takes and returns **degrees**. The engine converts to/from radians internally.
- `opts` (optional): A table containing:
  - `absolute`: boolean (default `false`). If `true`, the override replaces the animation transform entirely. If `false`, it is added on top of the current animation (ideal for head look and other additive overrides).
  - `interpolation`: float (default `0.0`, or the bone's persistent smooth value if one has been set). The time in seconds to smoothly transition to the new transform value.

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
- Passing `nil` as the table clears all overrides for that bone.
- `table` supports the following fields. Transform fields use a sub-table with a `vec` key:

```lua
player:set_bone_override("RightArm", {
    position = { vec = {x=0, y=0, z=0}, absolute = false, interpolation = 0.0 },
    rotation = { vec = {x=0, y=0, z=0}, absolute = false, interpolation = 0.0, degrees = true },
    scale    = { vec = {x=1, y=1, z=1}, absolute = false, interpolation = 0.0 },
    visible     = true,
    pos_smooth  = 0.0,  -- persistent smooth for position
    rot_smooth  = 0.2,  -- persistent smooth for rotation
    scale_smooth = 0.0, -- persistent smooth for scale
    color = "#FFFFFF",  -- ColorSpec: tints the bone mesh
    glow  = 0.0,        -- float: adds emissive glow to the bone mesh
})
```

- **`rotation.vec` defaults to radians**, but if `degrees = true` is passed in the rotation sub-table, it will be interpreted as degrees.

### Querying transforms

`ObjectRef:get_bone_position(bone) -> position, rotation`
- Returns **two** vectors for legacy compatibility: the position `{x,y,z}` and the rotation `{x,y,z}` **in degrees**.
- To read only position, discard the second return value.

`ObjectRef:get_bone_rotation(bone) -> rotation`
- Returns a single vector `{x,y,z}` with the current rotation override in **degrees**.

`ObjectRef:get_bone_scale(bone) -> scale`
- Returns a single vector `{x,y,z}` with the current scale override.

`ObjectRef:get_bone_override(bone) -> table`
- Returns the full override table for a single bone in the same format accepted by `set_bone_override`.
- `rotation.vec` is in **radians**.

`ObjectRef:get_bone_overrides() -> table`
- Returns a table keyed by bone name, each value being a full override table in the same format as `get_bone_override`.

`ObjectRef:get_bone_list() -> table`
- Returns a list (array) of all bone names found in the object's current mesh.
- Currently only supported for glTF/GLB models.

`ObjectRef:get_bone_world_pos(bone) -> vector`
- Returns the world-space vector of the rendered bone. Ideal for spawning particles or effects attached to specific body parts. Available on both server-side `ObjectRef` and client-side `core.localplayer`.

### How it works

- **Independence**: Each transform (position, rotation, scale) is stored and synced independently. Calling `set_bone_rotation` only updates the rotation; existing position or scale overrides are left untouched.
- **Client-side blending**: Overrides are applied in the client-side rendering loop after glTF animation blending has occurred. This ensures that animation clips (like walking) do not reset manual overrides (like head looking) every frame.
- **Synchronization**: Changes made on the server are automatically serialized and sent to all observing clients.
- **Network protocol**: Bone overrides are sent via `AO_CMD_SET_BONE_POSITION`. The `hidden` visibility state is packed into bit 3 of the flags byte, and smoothing values are appended to the packet. This approach maintains backward compatibility with older clients.
- **Euler persistence**: The API stores the exact Euler angles you provide, avoiding gimbal lock or "twisting" issues that often occur when converting back and forth between quaternions and Euler angles.

This implementation enables robust Minecraft-style head movement, procedural animations, modular entity attachments, and per-part visibility control for equipment systems.

---

## Lua Animator (`core.animator`)

`core.animator.create(object, def)`
- State machine + events + additive layers.
- `def` fields:
  - `states`: table of state definitions (each with `clip`, `range`, `speed`, `loop`, `blend`, `events`).
  - `transitions`: array of transition rules (`from`, `to`, `priority`, `condition`, `blend`).
  - `initial`: string — name of the starting state (defaults to an arbitrary first state key if not set).
  - `initial_blend`: number — blend duration for the initial state transition.
  - `get_context`: `function(animator, object, dtime) -> ctx` — returns the context table used to evaluate transitions. Defaults to `{vel, hs, moving}`.
  - `on_event`: `function(animator, object, event_payload)` — called for events fired by this animator instance.
  - `on_step`: `function(animator, object, dtime, ctx)` — called each update step.

`core.animator.register(animator)`
- Auto-updates each globalstep. Returns the animator.

`core.animator.unregister(animator)`
- Stops auto-updating.

### State control and introspection

`animator:get_current_state() -> string`
- Returns the name of the active state.

`animator:queue_state(name, opts?)`
- Adds a state to the playback queue.
- Queued states only play after the current non-looping state finishes.
- If the current state is looping, the queue will not be processed unless a transition occurs.
- Transitioning to a new state via `set_state` or a priority rule clears the queue by default.

`animator:get_queue() -> table`
- Returns the list of queued states.

### Global animator event bus

`core.animator.register_on_event(cb)`

`core.animator.unregister_on_event(cb)`
- Returns `true` if the callback was found and removed, `false` otherwise.
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
- `clips`: table of clip identifiers (number index or string name) keyed by state name: `idle`, `walk`, `run`, `jump`, `attack`. Each value can be a bare clip identifier or a full state definition table.
- `opts`:
  - `walk_threshold`: number (default `0.05`) — horizontal speed to transition from idle to walk.
  - `run_threshold`: number (default `2.5`) — horizontal speed to transition from walk to run.
  - `initial`: string (default `"idle"`) — starting state.
  - `initial_blend`: number — blend for the initial transition.
  - `get_context`: `function(animator, object, dtime) -> ctx` — must set `ctx.jumping` and/or `ctx.attack` for jump/attack transitions to fire. Defaults to horizontal-speed-only context.
  - `on_event`: `function(animator, object, event)` — local event callback.
  - `on_step`: `function(animator, object, dtime, ctx)` — per-step callback.

### Animation end helper

`core.on_animation_end(object, cb)` (alias for `core.animator.on_animation_end`)
- Calls `cb(object)` when the current non-looping animation is expected to end (computed from `ObjectRef:get_animation()`).
- Only fires for non-looping animations with non-zero speed.

`core.animator.cancel_on_animation_end(object)`
- Cancels a previously registered end watcher for the object.

### Animation cycle helper

`core.on_animation_cycle(object, cb)` (alias for `core.animator.on_animation_cycle`)
- Calls `cb(object)` each time a looping animation completes one full cycle (wraps around).
- Useful for syncing footstep sounds, particles, and other cyclic effects.

`core.animator.cancel_on_animation_cycle(object)`
- Cancels a previously registered cycle watcher for the object.

### glTF Animation Events

glTF models can contain named events at specific times within an animation. This fork extracts these from the glTF `extras` field and fires them during playback.

**glTF Structure:**
Events should be placed in the `extras` field of an animation object:
```json
"animations": [
  {
    "name": "Walk",
    "extras": {
      "events": [
        { "time": 0.5, "name": "footstep_l" },
        { "time": 1.0, "name": "footstep_r" }
      ]
    },
    "channels": [...]
  }
]
```

**Lua API:**
Register a global listener to receive these events:
`core.animator.register_on_event(function(animator, object, event))`
- `event.name`: The name defined in the glTF file.
- `event.engine`: `true` (indicates this is an engine-triggered model event).

Note: `animator` will be `nil` for events triggered directly by the engine's mesh node (when not using the Lua Animator state machine).

---

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
| `movement_speed_sprint_factor` | `1.3` | Multiplier for joystick/magnitude sprint |
| `movement_speed_crouch` | `1.3` | Speed while sneaking |
| `movement_acceleration_default` | `3.0` | Ground friction/responsiveness |
| `movement_acceleration_air` | `1.25` | Mid-air maneuverability |
| `movement_liquid_sink` | `0.4` | Downward speed in water |
| `movement_speed_climb` | `3.0` | Vertical ladder speed |

### Lua API (`player:set_physics_override`)

The full set of fields accepted and returned by `set_physics_override` / `get_physics_override`:

```lua
player:set_physics_override({
    speed                  = 1.0,   -- Multiplies walk/sprint speed
    jump                   = 1.0,   -- Multiplies jump speed
    gravity                = 1.0,   -- Multiplies gravity
    speed_climb            = 1.0,   -- Multiplies ladder climb speed
    speed_crouch           = 1.0,   -- Multiplies sneak speed
    acceleration_default   = 1.0,   -- Multiplies ground acceleration/friction
    acceleration_air       = 1.0,   -- Multiplies air acceleration
    speed_fast             = 1.0,   -- Multiplies fast-mode speed
    acceleration_fast      = 1.0,   -- Multiplies fast-mode acceleration
    speed_walk             = 1.0,   -- Multiplies base walk speed independently
    speed_sprint           = 1.0,   -- Multiplies the built-in 1.3x sprint boost
    step_height            = 1.0,   -- Multiplies step-up height
    liquid_fluidity        = 1.0,   -- Multiplies liquid fluidity
    liquid_fluidity_smooth = 1.0,   -- Multiplies liquid fluidity smoothing
    liquid_sink            = 1.0,   -- Multiplies liquid sink speed
    sneak                  = true,  -- Enables/disables sneaking
    sneak_glitch           = false, -- Enables legacy sneak-up-ledge glitch
    new_move               = true,  -- Enables new movement code
    auto_climb             = false, -- Enables auto-climb and auto-descend on ladders
})
```

### Node groups

- `group:lava`: Adding this to a node definition automatically enables the high-viscosity "Lava Physics."
- `group:viscous`: Value (1-7) sets custom exponential drag decay. Overrides standard liquid/lava physics if set.
- `group:disable_jump`: Prevents jumping while standing on or in the node.

### Player Callbacks

`core.register_on_jump(function(player))`
- Fired when a player performs a jump.
- `player`: `ObjectRef` of the player who jumped.

`core.register_on_land(function(player))`
- Fired when a player touches the ground after being in the air.
- `player`: `ObjectRef` of the player who landed.

---

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

---

## Fog API (Lua)

Extended volumetric and height-based fog controls.

`core.set_fog(player, params_or_nil)`
- Sets custom fog parameters for a specific player. Pass `nil` to clear.
- `params`:
  - `color`: ColorSpec (default: sky fog color)
  - `fog_start`: number (`0..0.99`, fraction of view distance; pass a negative value to leave at engine default)
  - `fog_end`: number (`0..1`, fraction of view distance; pass a negative value to leave at engine default; clamped to ≥ `fog_start` when both are non-negative)
  - `blend_time`: number (seconds, transition duration; clamped to ≥ `0`)
  - `max_density`: number (`0..1`, opacity at max height; clamped)
  - `max_density_height`: number (node-space height for max density)
  - `zero_density_height`: number (node-space height where fog disappears)
  - `uniform`: boolean (if true, ignores height density)
  - `direction`: v3f (up vector for height calculation, default `{x=0,y=1,z=0}`; normalized automatically)
  - `turbulence`: number (`0..1`, noise factor; clamped)
  - `speed_density_scale`: number (multiplier for density based on player speed; clamped to ≥ `0`)
  - `layers`: array of up to **4** extra fog layer tables (excess entries are silently dropped). Each layer supports: `color`, `max_density`, `max_density_height`, `zero_density_height`, `uniform`, `direction`.
  - `color_transition`: table (dynamic color animation):
    - `speed`: number (animation speed; clamped to ≥ `0`)
    - Up to **8** keyframes (excess entries are silently dropped), provided as an array directly in the table or in a `keyframes` sub-array:
      - `{ time = number(0..1), color = ColorSpec }`
    - Keyframes are automatically sorted by time after parsing.

`core.set_fog_boundary(player, params_or_nil)`
- Defines a localized fog zone. Pass `nil` to clear.
- `params`:
  - `pos`: v3f (center of the zone)
  - `radius`: number (node-space size; clamped to ≥ `0`)
  - `shape`: string (`"sphere"` (default), `"box"`, `"cylinder"`)
  - `fog`: table (FogParams structure as defined above)
  - `sound`: table (optional ambient sound inside zone):
    - `name`: string
    - `gain`: number (clamped to ≥ `0`)
    - `fade_in`: number (seconds; clamped to ≥ `0`)

`core.register_biome_atmosphere(biome_id, params)`
- Registers fog and/or boundary parameters for a specific biome.
- `biome_id`: integer
- `params`:
  - `fog`: table (FogParams)
  - `boundary`: table (FogBoundaryParams)

---

## World Switching API (Lua)

Allows automatically leaving the current world and joining another world by name.

`core.world_switch(worldname)` (Client-side)
- `worldname`: string — The name of the world to switch to.
- Automatically disconnects the user and joins the target world.
- Only works for local worlds.

`minetest.world_switch(playername, worldname)` (Server-side)
- `playername`: string — The name of the player to switch.
- `worldname`: string — The name of the world to switch to.
- Sends a request to the client to switch worlds.
- Only works if the client has the target world locally.

`core.create_world(name, gameid, options)` (Server-side)
- `name`: string — The name of the world to create.
- `gameid`: string — The game ID to use for the world.
- `options`: table — Optional settings for the world:
    - `seed`: string or number — The world seed.
    - `mg_name`: string — The map generator to use (e.g., `"v7"`, `"flat"`).
    - `visible`: string — `"visible"` or `"hidden"`. If hidden, the world will not appear in the player's world list.
    - `synchronizes`: string — A world name or absolute path to a world whose data (players, inventory, auth, mod storage) should be synchronized.
    - `mods` / `worldmods`: table or string — Mods to enable or copy.
        - If a string is provided, it is treated as a path to a directory whose contents (sub-folders) are expanded and copied individually into `worldmods/`.
        - If a table is provided:
            - Can be an array of mod names: `{"mod1", "mod2"}` (enables global mods).
            - Can be a table keyed by mod name with boolean values: `{mod1 = true, mod2 = false}`.
            - Can be a table keyed by mod name with string paths: `{bundled_mod = "mods/bundled_mod"}`.
                - If a path is provided, the mod directory is copied into the new world's `worldmods/` folder.
                - If the path points to a directory containing an `init.lua`, `mod.conf`, or `modpack.conf`, it is copied as a single unit.
                - Otherwise, if it is a directory, its contents (sub-folders) are expanded and copied individually into `worldmods/`.
                - Relative paths are resolved relative to the calling mod's directory (server-side only).
    - Any other key-value pair will be written directly to the `world.mt` file.
- Returns `success, path_or_error`.

`core.get_synchronized_worldpath()` (Server-side)
- Returns the absolute path to the world being synchronized with, or `nil` if synchronization is not enabled for the current world.

**Example:**
```lua
-- Creating a world with custom mods and settings
core.create_world("ProgrammaticWorld", "minetest", {
    seed = "12345",
    mg_name = "v7",
    -- 'worldmods' is an alias for 'mods'. Both can be used and are additive.
    -- String value: treats path as a directory of mods to expand into the world.
    worldmods = "bundled_mods", -- Expands all mods in "my_mod/bundled_mods/"
    mods = {
        default = true, -- Enable a global mod
        -- Specifically copy "my_mod/custom_mod_dir/" as "my_submod"
        ["my_submod"] = "custom_mod_dir"
    },
    creative_mode = "true" -- Arbitrary world.mt setting
})
```

---
- **More Soon!**
- Latest Update: May, 30, 2026
