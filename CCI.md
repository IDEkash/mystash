# Formspec draw_point API & UI Framework

The `draw_point` formspec element is a complete, **retained-mode 2D UI framework** inside Luanti (Minetest). It treats custom non-rectangular shapes as interactive, animatable, and transformable UI components.

## Element Syntax

```formspec
draw_point[name;points;fill_type;fill_value;radius;properties]
```

### Parameters

* **`name`**: `string` — Unique identifier for the element. Sent as the field name in `on_formspec_input` on interaction.
* **`points`**: `string` — A space-separated list of coordinate pairs `X,Y` representing the vertices of the closed shape. For example, `"0,0 4,0 4,4 0,4"` defines a square.
  * Coordinates seamlessly respect standard or `real_coordinates` settings of the active formspec.
* **`fill_type`**: `string` — Specifies how the interior of the shape is filled.
  * `"color"`: Fills the shape with a solid color.
  * `"image"`: Stretches and maps an image texture across the shape.
  * `"none"`: Transparent interior; only draws the boundary/outline of the shape.
* **`fill_value`**: `string` — The value corresponding to the chosen `fill_type`.
  * If `"color"`, this is a standard color specification (e.g., `#FF000080` for semi-transparent red or `blue`).
  * If `"image"`, this is the filename of the texture (e.g., `heart.png`).
  * If `"none"`, this field is ignored.
* **`radius`**: `float` (optional, default `0.0`) — The bending radius of the corner vertices.
  * If specified and `> 0`, corners are automatically smoothed using a high-fidelity quadratic Bezier curve subdivision.
  * The engine automatically clamps the radius to at most half of the adjacent edge lengths to prevent overshooting and visual glitches.
* **`properties`**: `string` (optional) — A list of comma-separated or space-separated option flags configuring advanced transforms, layering, touch, hold, release, and input behaviors. See **Properties Guide** below.

---

## Retained-Mode properties & Features

### 1. Runtime Transforms
You can specify starting translation, scale, and rotation transforms inside the properties field:
* **`px=OFFSET_X` / `py=OFFSET_Y`**: Specifies a pixel offset translation from its original coordinates.
* **`sx=SCALE_X` / `sy=SCALE_Y`**: Sets scale factors along X and Y axes relative to the shape centroid.
* **`rot=ANGLE`**: Sets rotation angle in degrees around the shape centroid.

### 2. Groups & Hierarchical Parenting
You can assign parent elements to groups so that child elements inherit position, scale, and rotation transforms recursively.
* **`parent=PARENT_NAME`**: Declares that this element belongs to `PARENT_NAME`. Moving, rotating, or scaling the parent group will transform all its children automatically!

### 3. Z-Order & Explicit Layering
You can specify explicit rendering depth/priority layers:
* **`z=PRIORITY` / `z_index=PRIORITY`**: Elements with higher Z priority values are rendered on top of elements with lower values, regardless of their formspec declaration order.

### 4. Interactive Drag & Drop
You can turn any shape into a drag-and-drop element, ideal for custom windows, slider thumbs, or Android controls.
* **`draggable`**: Enables precision non-rectangular dragging.
* Drag events are sent to the server under the payload:
  * `"drag_start"`: When dragging begins.
  * `"drag"`: Sent in real-time as the pointer moves.
  * `"drag_end"`: When dragging stops.

### 5. Smooth Property Animations
You can animate transitions smoothly with ease-in/out (smoothstep) curves.
* **`anim_duration=DURATION_MS`**: When a property changes (e.g., positions, rotation, scale, or color), the client automatically interpolates from its previous state to the new target values over the given duration!

---

## Event Payloads (Server-Side)

When an event is triggered, the field's `name` is sent in the `fields` table of `register_on_formspec_input` with various payloads depending on the active `properties`:

1. **Touch Inputs (`pressable`, `hold`, `release`)**
   * If `input` is **disabled**:
     * Clicking down sends: `fields[name] = "press"`
     * Dragging/holding sends: `fields[name] = "hold"`
     * Releasing sends: `fields[name] = "release"`
2. **Dragging (`draggable`)**
   * Drag-start sends: `fields[name] = "drag_start"`
   * Drag-move sends: `fields[name] = "drag"`
   * Drag-end sends: `fields[name] = "drag_end"`
3. **Styled Textbox / Input Mode (`input`)**
   * If `input` is **enabled**:
     * Typings and edits update the text dynamically on-screen.
     * The current typed string is sent under `fields[name]`. For example: `fields[name] = "hello world"`

---

## Modder Examples

### 1. Draggable Rounded Capsule Window

```lua
local caps_points = "0,0 8,0 8,4 0,4"
local formspec = "size[10,8]" ..
    "real_coordinates[true]" ..
    "draw_point[my_window;" .. caps_points .. ";color;#202030EE;1.0;draggable,z=100]"
```

### 2. Nested Hierarchy (Solar System)

Creates a sun and nested earth that inherits transforms from the sun.

```lua
-- Sun group at Z=10
local sun_points = "3,3 4,2 5,3 4,4"
local formspec = "size[8,8]" ..
    "real_coordinates[true]" ..
    "draw_point[sun;" .. sun_points .. ";color;#FFCC00FF;0.5;z=10,rot=45]" ..
    -- Earth grouped inside sun
    "draw_point[earth;1,1 2,1 2,2 1,2;color;#3366FFFF;0.2;parent=sun,z=20]"
```

### 3. Smooth Transition Animation

Animates a shape smoothly when formspec is updated.

```lua
-- Specifying anim_duration will smoothly grow the hexagon from zero scale to full target scale!
local hex_points = "1,0 3,0 4,2 3,4 1,4 0,2"
local formspec = "size[8,6]" ..
    "real_coordinates[true]" ..
    "draw_point[my_hex;" .. hex_points .. ";color;#FF0000FF;0.5;anim_duration=800,rot=180]"
```
