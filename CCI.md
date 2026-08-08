# Creative Composition Interface (CCI)

The **Creative Composition Interface (CCI)** is a built-in custom UI system for Luanti modders. It does not replace the traditional formspec engine; instead, it provides a high-level, shape-based, and object-oriented API that decouples interface geometry (structure) from styling, interactivity, animations, and behavioral logic.

Under the hood, CCI treats the standard formspec engine purely as a low-level graphics rasterizer. Modders write clean, modern, and vector-oriented Lua code while CCI automatically translates custom shapes into optimized rendering coordinates, dotted outline drawing loops, translucent/solid colors, textures, and overlays them with invisible click/touch target boxes.

---

## 1. Structural Architecture

CCI strictly enforces a separation between geometry structure and behavioral logic. A typical CCI layout is divided into two distinct parts:

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
- Adds a coordinate point to the current active shape session.
- `x`, `y`: numeric absolute coordinates. (Default coordinate scale maps `x * 0.1` to standard formspec columns/rows).
- Returns: A point table containing `{x, y, id}`.

`cci.connect(p1, p2)`
- Connects point `p1` to point `p2` with a line connection.
- `p1`, `p2`: Point tables returned from `cci.add_point`.

`cci.close(name)`
- Closes the active shape builder session, wraps all points and connections relative to the shape's calculated bounding box, and registers a global template named `name`.
- `name`: String identifier for the completed shape.
- Returns: An Object instance representing the newly closed layout.

---

### Retrieval and Sessions (Block B & Runtime)

`cci.get(name[, player_name])`
- Fetches a shape layout template or player-specific instance.
- `name`: The closed shape name string.
- `player_name`: (Optional) String name of the active player to fetch their active instance.
- Returns: The Object instance or `nil`.

`cci.on(obj, event, callback)`
- Binds an event listener to an object.
- `obj`: CCI Object instance.
- `event`: The event name string. Supported options: `"press"`, `"hold"`, `"release"`, `"text_changed"`, `"hover"`, `"leave"`, `"focus"`, `"unfocus"`.
- `callback`: `function(self, ...)` invoked when the event triggers.

`cci.show(player_name)`
- Instantiates player-specific copies of all global templates, creates a clean isolated session, and renders the custom interface to their screen.
- `player_name`: String name of the target player.

`cci.hide(player_name)`
- Closes the active CCI session and hides the layout from the player's screen.
- `player_name`: String name of the target player.

---

### Object Instance Methods

`obj:set(attribute, value)`
- Sets a style or action property on the object instance.
- **Style attributes**:
  - `"fill_color"`: Hex color string (e.g. `"#4CAF50"`, `"#FF000080"`, `"green"`).
  - `"fill_image"`: Texture file string (e.g. `"check.png"`).
  - `"transparency"`: Float value between `0.0` (fully transparent) and `1.0` (fully opaque).
  - `"blur"`: Numeric blur radius (stored conceptually).
- **Action attributes**:
  - `"pressable"`: Boolean. Enables press callbacks.
  - `"hold"`: Boolean. Enables hold callbacks.
  - `"release"`: Boolean. Enables release callbacks.
  - `"input"`: Boolean. Transforms the overlay into an editable text field.
- **State attributes**:
  - `"visible"`: Boolean. Sets visibility of the shape.

`obj:get(attribute)`
- Retrieves the current value of an attribute.

`obj:set_state(name, value)`
- Stores an arbitrary logic state on the object (e.g., checkbox toggles).
- `name`: State key name string.
- `value`: State value.

`obj:get_state(name)`
- Retrieves an arbitrary logic state from the object.

`obj:show()`
- Sets object visibility to `true`.

`obj:hide()`
- Sets object visibility to `false`.

`obj:destroy()`
- Removes the object from the player session (or the template registry if invoked globally).

`obj:animate(property, {to, duration, easing})`
- Smoothly animates any numeric or color property on the object instance.
- `property`: Attribute string (e.g., `"x"`, `"scale"`, `"transparency"`, `"fill_color"`).
- `to`: Target value (number or hex color string).
- `duration`: Float duration in seconds.
- `easing`: Easing algorithm name string. Supported values: `"linear"` (default), `"sine"`, `"smoothstep"`.

---

## 3. Interactive Demonstration

A fully interactive checkbox, textbox, and button demo can be experienced directly in-game. Ensure the `cci` mod is enabled (it registers automatically as a built-in module) and execute the following chat command:

```
/cci_test
```

### Script Example of the Demonstration:

```lua
-- Block A: Geometry construction
local p1 = cci.add_point(20, 20)
local p2 = cci.add_point(120, 20)
local p3 = cci.add_point(120, 60)
local p4 = cci.add_point(20, 60)
cci.connect(p1, p2)
cci.connect(p2, p3)
cci.connect(p3, p4)
cci.connect(p4, p1)
cci.close("play_button")

-- Block B: Attributes, States, and Events
local btn = cci.get("play_button")
btn:set("fill_color", "#4CAF50")
btn:set("pressable", true)

cci.on(btn, "press", function(self)
    -- Smoothly scale down on tap
    self:animate("scale", {to = 0.9, duration = 0.1, easing = "sine"})

    -- Rebound scale after click delay
    minetest.after(0.15, function()
        self:animate("scale", {to = 1.0, duration = 0.1, easing = "sine"})
    end)
end)
```
