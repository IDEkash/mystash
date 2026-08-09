# Formspec draw_point API

This fork introduces a brand-new, powerful, and highly performant formspec UI element called `draw_point`. This element goes beyond standard rectangles by allowing modders to define arbitrary closed polygon shapes, fill them with solid colors or textures with transparency, apply curved rounded corners (radius), capture precision pointer inputs (press, hold, release) constrained perfectly to the polygon bounds, and even use them as styled input/textboxes.

## Element Syntax

```formspec
draw_point[name;points;fill_type;fill_value;radius;properties]
```

### Parameters

* **`name`**: `string` — Unique identifier for the element. Used when reporting interactions/values back to the server in `on_formspec_input`.
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
* **`properties`**: `string` (optional) — A list of comma-separated or space-separated option flags configuring touch, hold, release, and input behaviors.
  * `pressable`: Triggers an event when the mouse/touch is pressed down inside the shape.
  * `hold`: Triggers events periodically while dragging/holding down inside the shape.
  * `release`: Triggers an event when the mouse/touch is released.
  * `input`: Enables styled textbox mode. Clicking the shape sets focus to it, allowing the player to type directly into the shape. Backspace is supported, and a blinking cursor `|` is drawn.

---

## Precision Non-Rectangular Hit Detection

Unlike standard bounding-box UI elements, the `draw_point` element uses a precise ray-casting/crossing-number algorithm to detect clicks and touches. A press, hold, or release event is **only** triggered if the pointer is mathematically inside the filled shape (or its rounded subdivisions). Clicking on the empty corners of the bounding box will correctly pass through to underlying elements!

---

## Event Payloads (Server-Side)

When an event is triggered, the field's `name` is sent in the `fields` table of `register_on_formspec_input` with various payloads depending on the active `properties`:

1. **Touch Inputs (`pressable`, `hold`, `release`)**
   * If `input` is **disabled**:
     * Clicking down sends: `fields[name] = "press"`
     * Dragging/holding sends: `fields[name] = "hold"`
     * Releasing sends: `fields[name] = "release"`
2. **Styled Textbox / Input Mode (`input`)**
   * If `input` is **enabled**:
     * Typings and edits update the text dynamically on-screen.
     * The current typed string is sent under `fields[name]`. For example: `fields[name] = "hello world"`

---

## Modder Examples

### 1. Rounded Interactive Hexagon Button

Draws a beautiful semi-transparent green rounded hexagon button that reports click and release events to the mod.

```lua
local hexagon_points = "1,0 3,0 4,2 3,4 1,4 0,2"
local formspec = "size[8,6]" ..
    "real_coordinates[true]" ..
    "draw_point[my_hex_btn;" .. hexagon_points .. ";color;#00FF00AA;0.5;pressable,release]"

minetest.register_on_formspec_input(function(player, formname, fields)
    if fields.my_hex_btn == "press" then
        minetest.chat_send_player(player:get_player_name(), "Hexagon pressed down!")
    elseif fields.my_hex_btn == "release" then
        minetest.chat_send_player(player:get_player_name(), "Hexagon released!")
    end
end)
```

### 2. Custom Rounded Textbox

A custom rounded capsule-shaped input field that can be typed into directly.

```lua
local capsule_points = "0,0 6,0 6,2 0,2"
local formspec = "size[8,4]" ..
    "real_coordinates[true]" ..
    "draw_point[my_textbox;" .. capsule_points .. ";color;#1E1E1EFF;1.0;input]"

minetest.register_on_formspec_input(function(player, formname, fields)
    if fields.my_textbox then
        minetest.chat_send_player(player:get_player_name(), "Current text: " .. fields.my_textbox)
    end
end)
```

### 3. Image-Filled Polygon

Stretches a background or sprite texture onto a custom triangle shape with rounded edges.

```lua
local triangle_points = "3,0 6,4 0,4"
local formspec = "size[8,6]" ..
    "real_coordinates[true]" ..
    "draw_point[my_triangle;" .. triangle_points .. ";image;default_stone.png;0.3;]"
```
