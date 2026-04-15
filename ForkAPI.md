# Fork APIs

This fork adds Android `htmlview` (including headless workers + JSON helpers), extra animator helpers, glTF multi-clip animation support, an independent bone transform API, upgraded animation blending with smoothstep easing and event callbacks, a state-machine animation controller, a refined physics and movement model, and an accessibility sprint toggle.

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

`htmlview.pipe(from_id, to_id)`
- Forwards messages from one HTMLView instance to another.

### Navigation / JS

`htmlview.navigate(id, url)`

`htmlview.inject(id, js)`

`htmlview.reload(id)`
- Reloads without destroying the instance.
- For `run_external*`, reloads the current entry.
- For `run*`, reloads the last provided HTML.

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

## glTF inspection helpers (Lua)

`core.gltf_get_animation_clips(path) -> list`
- Returns `{ {index,name,start,end,duration}, ... }`.

`core.gltf_inspect(path) -> table`
- Returns:
  - `meshes`: `{ {index,name,primitives}, ... }`
  - `bones`: `{ {node,name}, ... }` (joint nodes across skins)
  - `animations`: `{ {index,name,start,end,duration}, ... }`

## Independent Bone Transform API (Lua)

The independent bone transform API is fully implemented and available on all `ObjectRef` (players and entities). Each transform type (position, rotation, scale) is stored and synced independently — calling `setbonerotation` will only update the rotation and leave existing position or scale overrides untouched.

### Setting transforms

`ObjectRef:setboneposition(bone, position, opts?)`

`ObjectRef:setbonerotation(bone, rotation, opts?)`

`ObjectRef:setbonescale(bone, scale, opts?)`

**Arguments:**

- `position` / `rotation` / `scale`: Can be a table `{x=..., y=..., z=...}` or three separate numbers `x, y, z`. `setbonescale` also supports a single number for uniform scaling.
- `opts` (optional): A table containing:
  - `absolute`: boolean (default `false`). If `true`, the override replaces the animation transform entirely. If `false`, it is added on top of the current animation (ideal for head look and other additive overrides).
  - `interpolation`: float (default `0.0`). The time in seconds to smoothly transition to the new transform value.

### Querying transforms

`ObjectRef:getboneposition(bone)`

`ObjectRef:getbonerotation(bone)`

`ObjectRef:getbonescale(bone)`

Each returns a single vector (`{x,y,z}`) representing the current override for that specific transform.

### How it works

- **Independence**: Each transform (position, rotation, scale) is stored and synced independently. Calling `setbonerotation` only updates the rotation; existing position or scale overrides are left untouched.
- **Client-side blending**: Overrides are applied in the client-side rendering loop after glTF animation blending has occurred. This ensures that animation clips (like walking) do not reset manual overrides (like head looking) every frame.
- **Synchronization**: Changes made on the server are automatically serialized and sent to all observing clients.
- **Euler persistence**: The API stores the exact Euler angles you provide, avoiding gimbal lock or "twisting" issues that often occur when converting back and forth between quaternions and Euler angles.

This implementation enables robust Minecraft-style head movement, procedural animations, and modular entity attachments.

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
core.animator.registeronevent(function(animator, object, event)
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

## State-Machine Animation Controller (Lua)

A powerful state-machine animation system that allows defining named animation states with automatic conditional transitions and synchronization callbacks.

### Key features

1. **State machine**: Define named states with frame ranges (or `frames` alias), speed, and looping.
2. **Conditional transitions**: Automatically switch states using Lua functions that evaluate object context (e.g., velocity, controls).
3. **Synchronization callbacks**: New `core.animator` methods (`on_animation_start`, `on_animation_end`, etc.) allow syncing sounds or effects perfectly with animation states.
4. **Smooth blending**: Leverages the engine's animation blending (quaternion SLERP) during state transitions.
5. **Zero-overhead integration**: Uses a weak-keyed registry and metatable patching to add functionality to `ObjectRef` without breaking existing mod compatibility. Manual calls to `set_animation` correctly take precedence.

### API

```lua
entity:set_animation_controller({
    states = {
        idle = {frames={0, 20}, speed=30, loop=true},
        attack = {frames={21, 40}, speed=60, loop=false}
    },
    transitions = {
        {from="idle", to="attack", condition=function(ctx) return ctx.punching end}
    }
})
```

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
  - **Fall clamp**: When on a ladder, your maximum downward speed is clamped to -0.15, creating a slow, controlled slide.
  - **Forward boost**: If you press "Forward" while on a ladder, you get an upward boost matching your climb speed, allowing for faster ascending.
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
    acceleration_default = 1.0  -- Multiplies ground friction
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
