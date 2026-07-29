# Multi-Camera Rendering System & Render Target Pipeline

This document details the architecture, design, and API specification for the engine's Multi-Camera Rendering System and GPU Render Target (Framebuffer) Pipeline.

---

## 1. System Architecture

The Multi-Camera Rendering System allows multiple independent, configurable cameras to exist in the scene graph simultaneously, rendering either to off-screen textures (GPURenderTargets) or directly to custom-defined viewports on the screen.

```
Update Scene
→ Update Cameras (Parent transforms updated via Scene Graph)
→ For each active Camera:
   ├─ Update ClientMap camera/culling variables to match this camera
   ├─ Force ClientMap::updateDrawList() to populate the visible voxel block draw list
   ├─ Set active Irrlicht camera node
   ├─ Set viewport and active Render Target (or backbuffer viewport)
   ├─ Apply Render Layers/Masks (visibility culling)
   ├─ Render Scene via SceneManager::drawAll()
   └─ Restore original Scene visibilities
→ Render Main Camera (Standard pipeline)
→ Present Frame
```

---

## 2. Lua API Reference (Client-Side Modding)

The entire system is exposed to Lua through Client-Side Modding (CSM) APIs under the global `core` table.

### 2.1 GPU Render Targets (`GPURenderTarget`)

Creates a texture-backed framebuffer that can be rendered to by any camera, and bound to meshes or materials as a standard texture.

#### `core.create_render_target(width, height, format)`
- **Parameters:**
  - `width` (int): Width of the render target texture in pixels.
  - `height` (int): Height of the render target texture in pixels.
  - `format` (string, optional): Color format. Supported values:
    - `"rgba8"` (Default, 32-bit color with alpha)
    - `"rgb8"` (24-bit color without alpha)
    - `"rgb565"` (16-bit color)
    - `"rgba16f"` (Half-precision float format)
    - `"rgba32f"` (Full-precision float format)
- **Returns:** `RenderTarget` userdata object.
- **Behavior:** This automatically registers the render target with the engine's global `TextureSource`. You can use the `RenderTarget` object or its texture name string directly in mesh/material definitions.

#### Methods on `RenderTarget`
- `target:get_name()`
  - Returns the unique string name of the texture (e.g., `"rt_tex_1"`).
- `target:get_width()`
  - Returns the width of the texture.
- `target:get_height()`
  - Returns the height of the texture.
- `tostring(target)`
  - Direct conversion to string yields the texture name (useful when passing targets to standard texture APIs).

---

### 2.2 Independent Cameras (`RenderCamera`)

Represents an arbitrary viewport or off-screen render pass.

#### `core.create_camera()`
- **Returns:** `RenderCamera` userdata object.
- **Behavior:** Adds a new camera to the engine's active render pass manager and inserts a new camera node into the Irrlicht scene graph.

#### Methods on `RenderCamera`
- `camera:set_pos(pos)` / `camera:set_pos(x, y, z)`
  - Sets the position of the camera in voxel space.
- `camera:get_pos()`
  - Returns the 3D position vector `{x, y, z}`.
- `camera:set_rotation(rot)` / `camera:set_rotation(pitch, yaw, roll)`
  - Sets the Euler angles of the camera in degrees.
- `camera:get_rotation()`
  - Returns the 3D rotation vector `{x, y, z}`.
- `camera:set_fov(degrees)`
  - Sets the field of view in degrees.
- `camera:get_fov()`
  - Returns the field of view in degrees.
- `camera:set_projection(projection_type)`
  - Configures projection type: `"perspective"` or `"orthographic"`.
- `camera:get_projection()`
  - Returns `"perspective"` or `"orthographic"`.
- `camera:set_near_far(near, far)`
  - Configures the near and far clipping planes.
- `camera:get_near_far()`
  - Returns `near, far`.
- `camera:set_viewport(viewport)` / `camera:set_viewport(x, y, w, h)`
  - Configures the normalized viewport coordinates `(0.0` to `1.0`) relative to the render target (or screen).
- `camera:get_viewport()`
  - Returns `{x, y, w, h}` table.
- `camera:set_render_priority(priority)`
  - Configures render order (cameras with lower priority are rendered first).
- `camera:get_render_priority()`
  - Returns the render priority.
- `camera:set_render_target(target)`
  - Binds a `RenderTarget` to this camera. Pass `nil` to render directly to the screen (useful for splitscreen/picture-in-picture viewports).
- `camera:get_render_target()`
  - Returns the assigned `RenderTarget` object or `nil`.
- `camera:set_enabled(boolean)`
  - Enables or disables the camera. Disabled cameras consume 0% GPU time.
- `camera:get_enabled()`
  - Returns whether the camera is enabled.
- `camera:set_update_frequency(frequency_seconds)`
  - Configures how often the camera renders. `0.0` renders every frame; `N > 0.0` limits rendering to every `N` seconds.
- `camera:get_update_frequency()`
  - Returns the update frequency in seconds.
- `camera:set_render_mask(mask)`
  - Sets a bitwise 32-bit render layer mask. Only scene nodes whose ID matches the mask are rendered (bit-wise `ID & mask != 0` check). Pass `0xFFFFFFFF` to render everything.
- `camera:get_render_mask()`
  - Returns the render layer mask.
- `camera:set_resolution_scaling(scale)`
  - Configures resolution multiplier for render target/viewport rendering.
- `camera:get_resolution_scaling()`
  - Returns the resolution scaling factor.
- `camera:set_parent(parent)`
  - Attaches the camera to another engine object so it inherits position/rotation/scaling updates naturally. Supported parameters:
    - `"player"`: Attaches to the local player base node.
    - `"head"`: Attaches to the local player head node.
    - `"camera"`: Attaches to the main client camera node.
    - `"root"` or `nil`: Attaches to the root scene node (detached absolute coordinates).
    - `id` (int): Attaches to an arbitrary client active object (CAO) / entity matching this integer ID.

---

## 3. Example Use Cases

### 3.1 CCTV / Security Camera (Render to Texture)

Create an off-screen camera rendering a security viewport, and project its output onto any block or entity mesh texture:

```lua
-- 1. Create a 512x512 Render Target
local cctv_texture = core.create_render_target(512, 512)

-- 2. Create the camera and bind it to the target
local security_cam = core.create_camera()
security_cam:set_render_target(cctv_texture)
security_cam:set_pos({x = 120, y = 15, z = -84})
security_cam:set_rotation({x = 30, y = 45, z = 0})

-- 3. Set the texture on any entity or mesh node
-- Since RenderTarget supports string conversion to its registered name:
monitor_mesh:set_texture(cctv_texture)
-- Or explicitly:
monitor_mesh:set_texture(cctv_texture:get_name())
```

### 3.2 Picture-in-Picture Minimap / Rear-view Mirror (Direct Viewport Overlay)

Render a rear-view mirror directly onto a designated portion of the screen:

```lua
-- 1. Create the overlay camera
local mirror_cam = core.create_camera()
mirror_cam:set_parent("player") -- inherit player yaw/movement
mirror_cam:set_pos({x = 0, y = 1.5, z = 0}) -- positioned at head
mirror_cam:set_rotation({x = 0, y = 180, z = 0}) -- look backward

-- 2. Configure the viewport to top-center of the screen
mirror_cam:set_viewport({x = 0.35, y = 0.02, w = 0.3, h = 0.15})
mirror_cam:set_render_priority(100) -- render after main screen is drawn
```
