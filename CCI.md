# Creative Composition Interface (CCI) — Technical Specification

The **Creative Composition Interface (CCI)** is a built-in custom UI system for Luanti modders. It does not replace the traditional formspec engine; instead, it provides a high-level, shape-based, and object-oriented API that decouples interface geometry (structure) from styling, interactivity, animations, and behavioral logic.

CCI is implemented natively inside the Luanti core C++ source (`src/script/lua_api/l_cci.cpp` and `l_cci.h`) and registered during server startup, exposing the globally accessible `cci` namespace.

---

## 1. Structural Architecture

CCI strictly enforces a separation between geometry structure and behavioral logic:

- **Block A (Geometry & Layout)**: Constructs points, connects them to form outlines, and closes loops into named objects. No styling, logic, or attribute customization occurs here.
- **Block B (Attributes & Behavioral Logic)**: References closed shapes by name using `cci.get("name")` and applies style properties, action triggers, motion/animations, and event handler callbacks.

```lua
-- BLOCK A (Geometry)
local p1 = cci.add_point(20, 20)
local p2 = cci.add_point(120, 20)
local p3 = cci.add_point(120, 60)
local p4 = cci.add_point(20, 60)
cci.connect(p1, p2)
cci.connect(p2, p3)
cci.connect(p3, p4)
cci.connect(p4, p1)
cci.close("play_button")

-- BLOCK B (Attributes & Logic)
local btn = cci.get("play_button")
btn:set("fill_color", "#4CAF50")
btn:set("pressable", true)

cci.on(btn, "press", function(self)
    self:animate("scale", {to = 0.9, duration = 0.1, easing = "sine"})
end)
```

---

## 2. API Reference

### Builder Functions (Block A)

`cci.add_point(x, y)`
- Adds an absolute coordinate point to the current active builder session.
- `x`, `y`: Absolute numbers. Under the hood, these coordinates are scaled by `cci.scale` (default `0.1`) when rendered.
- **Returns**: A table containing the point data:
  ```lua
  { x = x, y = y, id = point_id }
  ```

`cci.connect(p1, p2)`
- Connects point `p1` to point `p2` to form a vector line boundary.
- `p1`, `p2`: The point tables returned by `cci.add_point` (must contain the `id` key).
- **Returns**: `nil`.

`cci.close(name)`
- Closes the active builder session, wraps all added points and connections into relative coordinates based on the shape's calculated bounding box, resets the builder, and registers a global template named `name`.
- `name`: A unique string identifier.
- **Returns**: An Object instance representing the newly registered shape template.

---

### Retrieval and Sessions (Block B & Runtime)

`cci.get(name[, player_name])`
- Fetches a shape template globally or a player-specific instance.
- `name`: The string identifier of the closed shape.
- `player_name`: (Optional) String name of the player whose active session contains the shape instance.
- **Returns**: The matching Object instance, or `nil` if not found.

`cci.on(obj, event, callback)`
- Binds an event callback function to the object's `callbacks` table.
- `obj`: CCI Object instance.
- `event`: Event string. Supported values:
  - `"press"`: Fired on clicking/tapping pressable shapes.
  - `"hold"`: Fired during hold interactions on pressable shapes.
  - `"release"`: Fired when clicking/tapping is released.
  - `"text_changed"`: Fired when text input fields are modified.
- `callback`: The callback function:
  - For `"press"`, `"hold"`, and `"release"`, the callback signature is `function(self)`.
  - For `"text_changed"`, the callback signature is `function(self, new_value)`.
- **Returns**: `nil`.

`cci.show(player_name)`
- Clones all global shape templates to instantiate a player-specific CCI session and renders the custom interface on the player's screen.
- `player_name`: String name of the target player.
- **Returns**: `nil`.

`cci.hide(player_name)`
- Closes the active CCI session, cleans up session memory from the server to prevent leaks, and closes the formspec on the player's screen.
- `player_name`: String name of the target player.
- **Returns**: `nil`.

---

### Object Instance Methods

Every closed shape returned by `cci.get` or `cci.close` is an Object instance possessing the following metatable methods:

`obj:set(attribute, value)`
- Sets an attribute property directly on the object.
- **Supported Style Attributes**:
  - `"fill_color"`: Hex color string (formats: `"#RRGGBB"`, `"#RRGGBBAA"`, `"#RGB"`, `"#RGBA"`) or standard named color strings (e.g. `"red"`, `"green"`, `"blue"`, `"white"`, `"black"`).
  - `"fill_image"`: Texture filename string (e.g. `"check.png"`).
  - `"transparency"`: Float opacity multiplier between `0.0` (invisible) and `1.0` (opaque).
  - `"blur"`: Conceptual numeric blur value.
- **Supported Interactivity Attributes**:
  - `"pressable"`: Boolean. Enables press/hold/release callbacks on invisible tap targets.
  - `"hold"`: Boolean. Enables hold callbacks.
  - `"release"`: Boolean. Enables release callbacks.
  - `"input"`: Boolean. Transforms the overlay into an editable text field.
- **Supported State Attributes**:
  - `"visible"`: Boolean. Toggles visibility of the shape.
  - `"x"`, `"y"`: Absolute coordinate numbers.
  - `"scale"`: Float scale multiplier (defaults to `1.0`).
- **Returns**: `nil`.

`obj:get(attribute)`
- Retrieves the current value of an attribute.
- **Returns**: The attribute value (boolean, string, or number) or `nil`.

`obj:set_state(name, value)`
- Stores an arbitrary logic state variable on the object (ideal for check state, slider position, etc.).
- `name`: A state key string.
- `value`: State value.
- **Returns**: `nil`.

`obj:get_state(name)`
- Retrieves an arbitrary logic state from the object.
- **Returns**: The state value or `nil`.

`obj:show()`
- Helper method to set `visible` property to `true`.

`obj:hide()`
- Helper method to set `visible` property to `false`.

`obj:destroy()`
- Permanently deletes the object from the player's active session (or templates registry).

`obj:animate(property, anim_def)`
- Smoothly animates a property's value over a given duration. This can animate coordinate values (`"x"`, `"y"`), scaling (`"scale"`), transparency (`"transparency"`), or colors (`"fill_color"`).
- `property`: Property name string (e.g. `"scale"` or `"fill_color"`).
- `anim_def`: A table containing exactly the following keys:
  ```lua
  {
      to = target_value,       -- Target number or hex color string
      duration = seconds,     -- Duration of animation in seconds
      easing = mode_string    -- Easing mode: "linear" or "sine" (smoothstep)
  }
  ```
- **Returns**: `nil`.

---

## 4. Built-in Demonstration

A fully interactive checkbox, textbox, and button demo can be loaded directly in-game. Ensure the `cci` mod is enabled (it registers automatically as a built-in module) and execute the following chat command:

```
/cci_test
```
