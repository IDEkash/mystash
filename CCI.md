# Creative Composition Interface (CCI)

This document describes the design, API, and implementation of the **Creative Composition Interface (CCI)**—a modern, high-performance UI system developed for Luanti as an alternative to the traditional formspec UI system.

---

## 1. Overview & Architecture

Unlike traditional UI systems that rely on predefined, hardcoded widgets (like Buttons, Sliders, Checkboxes), CCI is built on the philosophy of **creative composition**. Modern interfaces are assembled at runtime from simpler, modular components:

*   **Object Interface (Chapter 2):** Defines geometric bounds, points, closed-shape chains, and spatial structures. Objects have no built-in logic or styles—they simply represent spatial coordinates in a parent-child hierarchy.
*   **Attributes (Chapter 3):** Modify an Object Interface to add visual style (via CSS properties), interaction capabilities (Action Attributes like Pressable, Hold, Drag, Textbox), or animations (Motion Attributes).
*   **Functionable Services (Chapter 1):** The "brain" of CCI. Handles execution pipelines (detecting Events, evaluating Conditions/If statements, and triggering Actions).
*   **Runtime & Rendering Pipeline (Chapter 4):** Manages object lifecycles, computes recursive transforms (X, Y, Depth/Z, Rotation, Scale), handles safe-area clipping, and renders the hierarchy using an optimized WebView (`htmlview`) backed by GPU-accelerated CSS 3D.
*   **EasyTools (Chapter 5):** A creation toolkit providing helper functions (Auto Shapes) to instantly instantiate common geometric shapes (Rectangles, Circles, Stars, etc.).

---

## 2. API Design & Specifications

The CCI API is completely object-oriented and written in Lua.

### Core Creation Methods

#### `cci.create_object(options)`
Instantiates a new Object Interface.
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
Recursively destroys the object and all of its nested children, cleaning up all references.

---

### EasyTools (Auto Shapes)

#### `cci.easytools.create_rectangle(width, height, options)`
Generates a normal point-draw rectangle.

#### `cci.easytools.create_rounded_rectangle(width, height, radius, options)`
Generates a rectangle with custom border-radius styling.

#### `cci.easytools.create_circle(radius, options)`
Instantiates a circle with generated mathematical points.

#### `cci.easytools.create_star(outer_radius, inner_radius, points_count, options)`
Generates a star geometry with the specified number of star tips.

---

## 3. How It Works Under the Hood

### The Rendering Pipeline & HTML/CSS Bridge
CCI avoids traditional native C++ widget rendering in favor of a hybrid approach:
1. When CCI is initialized, it launches a single fullscreen `htmlview` session (`cci_runtime_view`).
2. An initial HTML document is loaded, preparing a standard 3D perspective context (`perspective: 1000px; transform-style: preserve-3d;`) to support modern **2.5D rendering** effects like card flipping, floating panels, and depth offsets.
3. Every global-step (capped to 60fps), the runtime checks if `cci.is_dirty` is true.
4. If changed, the runtime serializes the active object tree, including properties, hierarchies, styles, and transforms, and updates the WebView using `htmlview.send_json`.
5. The lightweight Javascript renderer updates the DOM, applies CSS rules, and uses CSS 3D transforms (`translate3d()`, `rotate()`, `scale()`) for ultra-smooth rendering.

### The Input & Interaction Event Loop
1. When user inputs (touches/clicks/drags) occur inside the WebView, the Javascript layer captures coordinates and DOM boundaries.
2. The JS bridge forwards JSON messages to Luanti via `luanti.send()`.
3. The Luanti message listener in `runtime.lua` parses the payload, identifies the target object, and executes the compiled Lua **Functionable Services** pipeline.

---

## 4. Composition Examples

### 1. Simple Interactive Button
```lua
local btn = cci.easytools.create_rounded_rectangle(200, 50, 10, {
	x = 100, y = 100,
	style = {
		background = "#4d90fe",
		color = "#ffffff",
	}
})

local label = cci.create_object({ type = "textbox" })
label:set_style("content", "Click Me")
btn:add_child(label)

btn:on("press", function(self)
	self:set_scale(0.95) -- Motion animation
end)
btn:on("release", function(self)
	self:set_scale(1.0)
	minetest.chat_send_all("Button Clicked!")
end)
```

### 2. A Toggle Switch
```lua
local track = cci.easytools.create_rounded_rectangle(70, 36, 18, {
	style = { background = "#ccc" }
})
local handle = cci.easytools.create_circle(14, {
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
