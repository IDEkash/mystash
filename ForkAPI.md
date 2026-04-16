# Fork APIs

This fork adds Android `htmlview` (including headless workers + JSON helpers), extra animator helpers, glTF multi-clip animation support, an independent bone transform API with smooth network interpolation, a refined physics & movement model, an accessibility sprint toggle, a state-machine animation controller, and an upgraded animation system with easing and events.

---

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

---

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

---

## glTF inspection helpers (Lua)

`core.gltf_get_animation_clips(path) -> list`
- Returns `{ {index,name,start,end,duration}, ... }`.

`core.gltf_inspect(path) -> table`
- Returns:
  - `meshes`: `{ {index,name,primitives}, ... }`
  - `bones`: `{ {node,name}, ... }` (joint nodes across skins)
  - `animations`: `{ {index,name,start,end,duration}, ... }`

---

## Lua Animator (`core.animator`)

`core.animator.create(object, def)`
- State machine + events + additive layers.

`core.animator.register(animator)`
- Auto-updates each globalstep.

### Global animator event bus

`core.animator.register_on_event(cb)` (alias: `core.animator.registeronevent`)
`core.animator.unregister_on_event(cb)`
- `cb(animator, object, event_payload)` called for every emitted animation event.

### Event types

The event table passed to your callback contains:

- `event.name`: The type of event.
  - `"jump_start"`: Fired when a humanoid begins a jump.
  - `"land"`: Fired when a humanoid finishes a jump and returns to the ground.
  - `"attack_start"`: Fired when an attack animation begins.
  - `"transition"`: Fired whenever the animator switches states.
- `event.from`: The state being left.
- `event.to`: The state being entered.
- `event.blend`: The duration of the blend.
- `event.ctx`: The animator context at the time of the transition.

**Example: Playing sounds on animation events**
```lua
core.animator.registeronevent(function(animator, object, event)
    if event.name == "jump_start" then
        minetest.sound_play("jump", {object = object})
    elseif event.name == "land" then
        minetest.sound_play("land", {object = object})
    end
end)
```

### Humanoid helper

`core.animator.humanoid(object, clips, opts?) -> animator`
- Builds a basic `idle/walk/run/jump/attack` state machine.
- Uses default context `hs` (horizontal speed). For `jump`/`attack`, provide `opts.get_context` that sets `ctx.jumping` / `ctx.attack`.

### Animation end helper

`core.on_animation_end(object, cb)` (alias for `core.animator.on_animation_end`)
- Calls `cb(object)` when the current non-looping animation is expected to end (computed from `ObjectRef:get_animation()`).

---

## Animation System Upgrade

The engine-level animation system has been upgraded for smoother, more natural transitions.

- **Smoothstep Easing**: The core rendering engine (`irr/src/AnimatedMeshSceneNode.cpp`) now uses smoothstep (ease-in/out) blending instead of linear blending during animation transitions.
- **Automatic Blend Default**: `set_animation` and `set_animation_clip` now default to a `0.1s` blend time if none is specified. Existing mods automatically benefit without any code changes.
- **Improved Humanoid Logic**: The humanoid animator helper more accurately tracks state changes for jumping and landing, ensuring events fire at the correct moment.

---

## State-Machine Animation Controller

A powerful Lua-side state-machine animation system available on all entities.

### Key features

- **State Machine**: Define named states with frame ranges (or `frames` alias), speed, and looping.
- **Conditional Transitions**: Automatically switch states using Lua functions that evaluate object context (e.g., velocity, controls).
- **Synchronization Callbacks**: `core.animator` methods (`on_animation_start`, `on_animation_end`, etc.) allow syncing sounds or effects perfectly with animation states.
- **Smooth Blending**: Leverages the engine's animation blending (quaternion SLERP) during state transitions.
- **Zero-Overhead Integration**: Uses a weak-keyed registry and metatable patching to add functionality to `ObjectRef` without breaking existing mod compatibility. Manual calls to `set_animation` correctly take precedence.

### API

`entity:set_animation_controller(def)`
- `def.states`: table of named states, each with:
  - `frames` / `frame_range`: `{start, end}`
  - `speed`: number
  - `loop`: boolean
- `def.transitions`: list of `{from, to, condition}` entries where `condition(ctx) -> boolean`.

**Example**
```lua
entity:set_animation_controller({
    states = {
        idle   = {frames={0,  20}, speed=30, loop=true},
        attack = {frames={21, 40}, speed=60, loop=false}
    },
    transitions = {
        {from="idle", to="attack", condition=function(ctx) return ctx.punching end}
    }
})
```

---

## Independent Bone Transform API

Available on all `ObjectRef` (players and entities). Underscored and non-underscored method names both work.

### Setting transforms

`object:set_bone_position(bone, position, opts?)`
`object:set_bone_rotation(bone, rotation, opts?)`
`object:set_bone_scale(bone, scale, opts?)`

- `position` / `rotation` / `scale`: `{x=..., y=..., z=...}` or three separate numbers. `set_bone_scale` also accepts a single number for uniform scaling.
- `opts` (optional):
  - `absolute`: boolean (default `false`). If `true`, the override replaces the animation transform. If `false`, it is added on top (ideal for head-look).
  - `interpolation`: float (default `0.0`). Seconds to smoothly transition to the new transform.

### Querying transforms

`object:get_bone_position(bone) -> vector`
`object:get_bone_rotation(bone) -> vector`
`object:get_bone_scale(bone) -> vector`

Each returns the current override for that specific transform as `{x, y, z}`.

### How it works

- **Independence**: Each transform (position, rotation, scale) is stored and synced independently. Calling `set_bone_rotation` leaves existing position or scale overrides untouched.
- **Client-side blending**: Overrides are applied after glTF animation blending, so animation clips (e.g., walking) do not reset manual overrides (e.g., head-look) every frame.
- **Synchronization**: Server-side changes are automatically serialized and sent to all observing clients.
- **Euler persistence**: Exact Euler angles are stored, avoiding gimbal lock or "twisting" issues from quaternion round-trips.

---

## Smooth Bone Interpolation

Network-transparent, render-rate bone smoothing built on top of the Independent Bone Transform API.

### How it works

- **Client-side update tracking**: The client records the arrival time of each bone update packet and uses an Exponential Moving Average (EMA) to maintain a stable update interval (e.g., ~0.05 s at 20 Hz server tick rate).
- **Render-rate smoothing**: Instead of snapping to new values on each packet, the engine interpolates bone transforms every frame (~60 fps) using Quaternion SLERP for rotations, ensuring perfectly smooth angular transitions.
- **Prediction & jitter buffering**: Temporal extrapolation continues movement up to 150% past the last received value if the next packet is late, eliminating the stutter common with 20 Hz updates. A damping factor of `0.8×` absorbs small delivery variations without visible snapping.
- **Animator blending**: Interpolation is applied after the base animation, so smooth head tracking correctly blends with existing animations (walking, attacking, idle) unless `absolute = true` is used.

### API

The `interpolate` flag is available on all bone-related `ObjectRef` methods.

#### `set_bone_rotation` / `set_bone_position` / `set_bone_scale`

Pass `interpolate = true` in the options table to enable client-side smoothing.

```lua
-- Smooth head tracking
entity:set_bone_rotation("head", {x=pitch, y=yaw, z=0}, {
    interpolate = true,   -- enables smooth client-side smoothing
    absolute    = false,  -- false = relative to animation, true = override
})
```

#### `set_bone_override`

For complex setups, the flag is available per-property.

```lua
entity:set_bone_override("head", {
    rotation = {
        vec         = {x=pitch, y=yaw, z=0},
        interpolate = true,
        absolute    = false,
    },
    position = {
        vec         = {x=0, y=1, z=0},
        interpolate = true,
    }
})
```

#### `get_bone_override`

Returned tables now include the `interpolate` field so you can inspect the current smoothing state.

```lua
local override = entity:get_bone_override("head")
print(override.rotation.interpolate)  -- true or false
```

### Compatibility

- **Legacy support**: Omitting `interpolate` or setting it to `false` falls back to the original behavior (duration-based interpolation or instant snapping). Existing mods are unaffected.
- **Zero network overhead**: Flags are packed into existing protocol bytes — no increase in network traffic.

---

## Physics & Movement Model

A refined physics model inspired by high-performance mobile voxel engines, targeting a snappy and physical feel.

### How it works

**Physical gravity & jump**
- Gravity is `32.0 nodes/s²` — the hidden "Factor of 2" engine hack has been removed. Falling is fast and heavy.
- Jump speed is `9.5 nodes/s`, tuned to clear exactly one block with a small margin.

**Responsive movement**
- Ground acceleration (`3.0`) removes the "ice-skating" feel — the player starts and stops quickly.
- Air acceleration (`1.25`) provides balanced mid-air control without feeling floaty.

**Built-in sprint logic**
- If joystick / key input magnitude reaches ≥ 95% (fully pushed), walking speed is automatically multiplied by `1.3×` (~5.6 nodes/s sprint). Handled in the engine's `applyControl` step, so it works for any input device.

**Specialized liquid physics**
- Standard liquids: uses `movement_liquid_sink` (`0.4`) and fluidity (`0.2`) for a sluggish swim.
- Lava (`group:lava`): exponential velocity decay (`e^{-5t}`) per frame plus a forced downward sink of `0.5 blocks/s`.

**Advanced movement mechanics**
- *Ladder climbing*: Downward speed is clamped to `-0.15` for a controlled slide. Pressing Forward grants an upward boost matching climb speed.
- *Edge-grabbing (sneak)*: Instead of an invisible wall, velocity is reduced by 50% per frame at the sneak limit, creating a smooth edge-grab.

### Default settings (`minetest.conf`)

| Setting | Default | Description |
|---|---|---|
| `movement_gravity` | `32.0` | Global gravity (nodes/s²) |
| `movement_speed_jump` | `9.5` | Initial upward velocity for jump |
| `movement_speed_walk` | `4.3` | Baseline walking speed |
| `movement_speed_crouch` | `1.3` | Speed while sneaking |
| `movement_acceleration_default` | `3.0` | Ground friction / responsiveness |
| `movement_acceleration_air` | `1.25` | Mid-air maneuverability |
| `movement_liquid_sink` | `0.4` | Downward speed in water |
| `movement_speed_climb` | `3.0` | Vertical ladder speed |

### Lua API (`player:set_physics_override`)

```lua
player:set_physics_override({
    speed                = 1.0,  -- Multiplies walk/sprint
    jump                 = 1.0,  -- Multiplies jump speed
    gravity              = 1.0,  -- Multiplies gravity
    speed_climb          = 1.0,  -- Multiplies ladder speed
    acceleration_default = 1.0   -- Multiplies ground friction
})
```

### Node groups

- `group:lava` — Enables high-viscosity lava physics on a node.
- `group:disable_jump` — Prevents jumping while standing on or in the node.

---

## Accessibility Sprint Toggle

Allows players to disable the automatic joystick-driven sprint.

### How it works

- The `1.3×` speed multiplier in `src/client/localplayer.cpp` now only activates when `accessibilitysprintenabled` is `true`.
- The toggle in the **Accessibility → Movement** menu updates `PlayerSettings` in real-time — no restart required.
- The setting is registered in `src/defaultsettings.cpp` and defaults to `true` (preserving original behavior when missing from `minetest.conf`).

### API

**Lua**
```lua
core.settings:get_bool("accessibilitysprintenabled")
```

**C++ (engine internals)**
```cpp
player_settings.accessibility_sprint_enabled  // bool
```

---

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
