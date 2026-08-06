# Creative Composition Interface (CCI)

This document describes the design, API, and implementation of the **Creative Composition Interface (CCI)**—a modern, high-performance, multiplayer-safe UI system developed for Luanti as an alternative to the traditional formspec UI system.

---

## 1. Overview & Architecture

Unlike traditional UI systems that rely on predefined, hardcoded widgets (like Buttons, Sliders, Checkboxes), CCI is built on the philosophy of **creative composition**. Modern interfaces are assembled at runtime from simpler, modular components:

*   **Multiplayer-Safe Sessions:** Storage and updates are dynamically partitioned per connected player, allowing seamless multi-client rendering without state conflicts.
*   **Object Interface (Chapter 2):** Defines geometric bounds, points, closed-shape chains, and spatial structures. Objects have no built-in logic or styles—they simply represent spatial coordinates in a parent-child hierarchy.
*   **Attributes (Chapter 3):** Modify an Object Interface to add visual style (via CSS properties), interaction capabilities (Action Attributes like Pressable, Hold, Drag, Textbox), or animations (Motion Attributes).
*   **Functionable Services (Chapter 1):** The "brain" of CCI. Handles execution pipelines (detecting Events, evaluating Conditions/If statements, and triggering Actions).
*   **Runtime & Rendering Pipeline (Chapter 4):** Manages object lifecycles per player, computes recursive transforms (X, Y, Depth/Z, Rotation, Scale), handles safe-area clipping, and renders the hierarchy using an optimized WebView (`htmlview`) backed by GPU-accelerated CSS 3D.
*   **EasyTools (Chapter 5):** A creation toolkit providing helper functions (Auto Shapes) to instantly instantiate common geometric shapes (Rectangles, Circles, Stars, etc.).

---

## 2. API Design & Specifications

The CCI API is completely object-oriented, player-partitioned, and written in Lua.

### Player Session Management

#### `cci.get_session(player_name)`
Retrieves (or creates) the isolated session container for a specific player.
*   Returns a Session object containing helper methods.
```lua
local session = cci.get_session("singleplayer")
```

#### `Session:create_object(options)`
Convenience method to instantiate a new Object Interface tied directly to this session.

#### `Session:destroy()`
Recursively destroys all objects inside this player's session and stops the runtime WebView.

---

### Core Creation Methods

#### `cci.create_object(player_name, options)`
Instantiates a new Object Interface tied to a specific player's session.
*   `player_name` (string): The target player's name.
*   `options` (table, optional):
    *   `id`: String unique identifier.
    *   `type`: String indicating geometry type (`rectangle`, `circle`, `shape`, etc.).
    *   `x`, `y`, `z`: Initial spatial coordinates (Z represents depth for 2.5D rendering).
    *   `rotation`: Spatial rotation in degrees.
    *   `scale`: Spatial scale multiplier.
    *   `style`: Table containing CSS style key-value pairs.
    *   `layer`: Number representing rendering layer (z-index).
    *   `safe_area`: Boolean. If true, clips the rendering of all child objects.
    *   `visible`: Boolean indicating whether the object is rendered initially.

---

### Object Instance Methods

#### `Object:add_point(x, y)`
Adds a geometric vertex location to the shape's definition. Returns the index of the point.

#### `Object:add_chain(...)`
Links multiple point indices together to form a path or a closed shape.

#### `Object:set_style(key, value)`
Directly sets a custom style or CSS property on the element.
```lua
obj:set_style("background", "linear-gradient(45deg, #ff007f, #7f00ff)")
```

#### `Object:set_styles(tbl)`
Batch sets multiple CSS styles.

#### `Object:set_pos(x, y, z)`
Updates the current transform position.

#### `Object:set_rotation(r)`
Updates the current transform rotation in degrees.

#### `Object:set_scale(s)`
Updates the spatial scale of the object.

#### `Object:set_layer(layer)`
Sets rendering order layer.

#### `Object:set_safe_area(enabled)`
Enables/disables safe-area clipping for child elements.

#### `Object:add_child(child)`
Attaches a child object to this object, creating a parent-child relationship. Children inherit parent transforms and visibility.

#### `Object:remove_child(child)`
Detaches a child object.

#### `Object:on(event_name, [condition_fn], action_fn)`
Registers a Functionable Service handler.
*   `event_name`: `"press"`, `"release"`, `"drag_start"`, `"dragging"`, `"drag_end"`.
*   `condition_fn`: (Optional) Function returning boolean. If false, action execution is aborted.
*   `action_fn`: Function executed upon event receipt.

#### `Object:trigger(event_name, data)`
Manually fires an event on the object.

#### `Object:show()` / `Object:hide()`
Changes visibility of the object.

#### `Object:destroy()`
Recursively destroys the object and all of its nested children, cleaning up all references from the player session.

---

### EasyTools (Auto Shapes)

Every EasyTools creation method takes `player_name` as its first parameter to automatically register under the player's session.

#### `cci.easytools.create_rectangle(player_name, width, height, options)`
Generates a normal point-draw rectangle.

#### `cci.easytools.create_rounded_rectangle(player_name, width, height, radius, options)`
Generates a rectangle with custom border-radius styling.

#### `cci.easytools.create_circle(player_name, radius, options)`
Instantiates a circle with generated mathematical points.

#### `cci.easytools.create_star(player_name, outer_radius, inner_radius, points_count, options)`
Generates a star geometry with the specified number of star tips.

---

## 3. How It Works Under the Hood

### The Rendering Pipeline & HTML/CSS Bridge
CCI avoids traditional native C++ widget rendering in favor of a hybrid approach:
1. When a player joins, the runtime listener (`register_on_joinplayer`) launches a player-specific `htmlview` session (`cci_view_<player_name>`).
2. An initial HTML document is loaded, preparing a standard 3D perspective context (`perspective: 1000px; transform-style: preserve-3d;`) to support modern **2.5D rendering** effects like card flipping, floating panels, and depth offsets.
3. Every global-step (capped to 60fps), the runtime loops through active player sessions and checks if `session.is_dirty` is true.
4. If changed, the runtime serializes the active object tree for that player, and updates their client WebView using `htmlview.send_json` with their session-specific `view_id`.
5. The lightweight Javascript renderer updates the DOM, applies CSS rules, and uses CSS 3D transforms (`translate3d()`, `rotate()`, `scale()`) for ultra-smooth rendering.
6. When a player leaves, their session is destroyed and the WebView is cleanly stopped.

### The Input & Interaction Event Loop
1. When user inputs (touches/clicks/drags) occur inside the WebView, the Javascript layer captures coordinates and DOM boundaries.
2. The JS bridge forwards JSON messages to Luanti via `luanti.send()`.
3. The Luanti message listener in `runtime.lua` parses the payload, identifies the target session and object, and executes the compiled Lua **Functionable Services** pipeline.

---

## 4. Composition Examples

### 1. Simple Interactive Button
```lua
local player_name = "singleplayer"

local btn = cci.easytools.create_rounded_rectangle(player_name, 200, 50, 10, {
	x = 100, y = 100,
	style = {
		background = "#4d90fe",
		color = "#ffffff",
	}
})

local label = cci.create_object(player_name, { type = "textbox" })
label:set_style("content", "Click Me")
btn:add_child(label)

btn:on("press", function(self)
	self:set_scale(0.95) -- Motion animation
end)
btn:on("release", function(self)
	self:set_scale(1.0)
	minetest.chat_send_player(player_name, "Button Clicked!")
end)
```

### 2. A Toggle Switch
```lua
local player_name = "singleplayer"

local track = cci.easytools.create_rounded_rectangle(player_name, 70, 36, 18, {
	style = { background = "#ccc" }
})
local handle = cci.easytools.create_circle(player_name, 14, {
	x = 4, y = 4,
	style = { background = "#fff" }
})
track:add_child(handle)

local state = false
track:on("press", function(self)
	state = not state
	if state then
		self:set_style("background", "#4cd964")
		handle:set_style("transform", "translate3d(34px, 0px, 0px)")
	else
		self:set_style("background", "#ccc")
		handle:set_style("transform", "translate3d(0px, 0px, 0px)")
	end
end)
```
