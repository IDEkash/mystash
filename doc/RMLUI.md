# RmlUi Native Engine Integration Documentation

This document describes the design, architecture, and usage of the natively integrated **RmlUi** UI framework inside the Luanti engine. RmlUi is a modern, high-performance UI system that runs fully in parallel with Formspec, allowing modern styles, flexbox, gradients, rounded corners, transitions, and native elements without any breakage or modifications to existing mods.

---

## 1. Architecture Overview

RmlUi is compiled directly into the engine, making it a first-class citizen of Luanti's client-side renderer.

- **`RmlUiIrrlichtRenderer`**: Implements a highly optimized rendering backend subclassing `Rml::RenderInterface`. It translates compiled RmlUi vertex buffers directly into IrrlichtMT `video::S3DVertex` buffers, applying transform matrices and viewport scissoring natively.
- **`RmlUiSystemInterface`**: Integrates with the engine for logging, time querying, and native OS clipboard interaction.
- **`RmlUiFileInterface`**: Resolves virtual asset paths and custom URIs (e.g. `mod://`, `game://`, `world://`, `builtin://`) to the real file system.
- **`RmlUiManager`**: A singleton client-side coordinator that handles context initialization, document lifecycle, screen resizing, and event routing (such as mouse clicks, keyboard strokes, touches, multi-touches, and orientation adjustments).

---

## 2. Lua API Reference

Exposes the `core.rmlui` namespace (aliased as `minetest.rmlui` on both CSM and the Main Menu).

### Namespace Functions

#### `core.rmlui.create(doc_id)`
Creates and registers a new RmlUi document container on the current screen context.
- **Returns**: A `ui` document object wrapper.

#### `core.rmlui.destroy(doc_id)`
Destroys the specified document container, releasing all of its resources.

---

### Document Object (`ui`) Methods

#### `ui:load(path)`
Loads an `.rml` document from disk or virtual path. Resolves custom URIs.

#### `ui:load_string(rml_content)`
Loads an `.rml` document directly from a Lua string.

#### `ui:show()`
Displays the document on the screen.

#### `ui:hide()`
Hides the document from the screen.

#### `ui:close()`
Closes and destroys the document container.

#### `ui:set_position(x, y)`
Sets the inline style coordinate properties (`left`, `top`) in pixels.

#### `ui:set_size(w, h)`
Sets the inline style dimension properties (`width`, `height`) in pixels.

#### `ui:set_text(element_id, text)`
Sets the inner text content of the element with the given ID.

#### `ui:set_html(element_id, html_content)`
Sets the inner RML content (HTML) of the element with the given ID.

#### `ui:set_style(element_id, style_string)`
Applies inline style attributes (CSS style properties) to the element.

#### `ui:set_attribute(element_id, key, value)`
Sets the specified attribute on the element.

#### `ui:add_class(element_id, class_name)`
Applies a style class to the element.

#### `ui:remove_class(element_id, class_name)`
Removes the specified style class from the element.

#### `ui:focus([element_id])`
Focuses the document or the specified element.

#### `ui:blur([element_id])`
Blurs the document or the specified element.

#### `ui:reload()`
Reloads the document from its original path (extremely useful for fast visual debugging).

#### `ui:bring_to_front()`
Pulls the document to the front of the UI rendering stack.

#### `ui:capture()`
Dummy stub for capturing screenshots.

#### `ui:call_js(script)`
Dummy stub for script compatibility.

#### `ui:on(event_name, callback_fn)`
Registers a callback to listen for native bubbling events.
- **Supported Events**: `"click"`, `"mouseenter"`, `"mouseleave"`, `"keydown"`, `"keyup"`, `"submit"`, `"change"`, `"input"`.
- **Callback Signature**: `callback_fn(element_id)`

#### `ui:find(element_id)`
Locates the element with the given ID and returns an `element` wrapper object.

#### `ui:find_all(selector)`
Returns a Lua array of `element` wrapper objects matching the given CSS selector query.

---

### Element Object (`el`) Methods

The elements returned by `ui:find(id)` or `ui:find_all(selector)` support:
- `el:set_text(text)`
- `el:set_html(html)`
- `el:set_style(style)`
- `el:set_attribute(key, value)`
- `el:add_class(class_name)`
- `el:remove_class(class_name)`
- `el:focus()`
- `el:blur()`
- `el:find(id)` (relative element find)
- `el:find_all(selector)` (relative query selector)

---

## 3. List of Modified & Added Source Files

The following files have been modified or added to fully integrate RmlUi directly into the engine:

### Added Files
- **`src/client/rmlui_backend.h`**: Declares interfaces, document wrappers, event listeners, and the coordinator singleton.
- **`src/client/rmlui_backend.cpp`**: Implements the renderer, system, VFS filesys resolver, keyboard mapping, and touch processing.
- **`src/script/lua_api/l_rmlui.h`**: Declares the `core.rmlui` C++/Lua bindings interface.
- **`src/script/lua_api/l_rmlui.cpp`**: Implements C++ Lua methods and bootstraps the elegant, lightweight object-oriented Lua metatables.

### Modified Files
- **`src/CMakeLists.txt`**: Added `lib/RmlUi-master` subdirectory, configured RmlUi options, linked `rmlui_core` directly into `luanti`, and added includes.
- **`src/client/CMakeLists.txt`**: Added `rmlui_backend.cpp` to the client source build list.
- **`src/script/lua_api/CMakeLists.txt`**: Added `l_rmlui.cpp` to the client scripting source list.
- **`src/script/scripting_client.cpp`**: Included `l_rmlui.h` and initialized the `core.rmlui` namespace on game client startup.
- **`src/script/scripting_mainmenu.cpp`**: Included `l_rmlui.h` and initialized the `core.rmlui` namespace on Main Menu startup.
- **`src/client/render/plain.cpp`**: Injected RmlUi rendering after DrawHUD GUI environment calls so that RmlUi renders on top of the game HUD.
- **`src/gui/guiEngine.cpp`**: Injected RmlUi rendering into the main menu draw loop.
- **`src/client/inputhandler.cpp`**: Injected mouse and keyboard event interception at the top of `MyEventReceiver::OnEvent` to give RmlUi priority on inputs.
- **`src/client/renderingengine.cpp`**: Injected RmlUi automatic cleanup and destruction upon game/client termination.

---

## 4. Android Build Instructions

The engine has been integrated to compile RmlUi directly into the client shared library (`libluanti.so`) shipped inside the Android APK.

1. Install Android SDK, NDK, and Gradle.
2. In the repository root, standard Android builds will automatically include and compile RmlUi because of our `src/CMakeLists.txt` and `src/client/CMakeLists.txt` changes.
3. To build the APK from terminal:
   ```bash
   cd android/
   ./gradlew assembleRelease
   ```
4. Gradle automatically triggers CMake cross-compilation with the Android NDK, linking `rmlui_core` and packaging the compiled binaries directly inside the output APK.

---

## 5. Examples

All files for the example mod can be found under `mods/rmlui_example/`.

### Example RML (`mods/rmlui_example/ui/example.rml`)
```xml
<rml>
<head>
	<title>RmlUi Example Card</title>
	<link type="text/rcss" href="example.rcss"/>
</head>
<body class="window">
	<div id="container">
		<div id="header">
			<h1>RmlUi Integrated UI</h1>
			<button id="close_btn">X</button>
		</div>
		<div id="content">
			<p>This is a modern UI rendered using <strong>RmlUi</strong> natively inside the Luanti engine!</p>
			<div class="form-group">
				<label>Enter Name: </label>
				<input type="text" id="name_input" value="Luanti Player"/>
			</div>
			<button id="submit_btn" class="primary">Submit</button>
		</div>
	</div>
</body>
</rml>
```

### Example RCSS (`mods/rmlui_example/ui/example.rcss`)
```css
body.window {
	font-family: Arimo;
	font-weight: normal;
	font-style: normal;
	font-size: 16px;
	color: #ffffff;
}

#container {
	width: 450px;
	height: auto;
	margin: 100px auto;
	background: #1e293b;
	border: 2px solid #38bdf8;
	border-radius: 12px;
	box-shadow: 0px 10px 25px rgba(0, 0, 0, 0.5);
}

#header {
	display: flex;
	justify-content: space-between;
	align-items: center;
	background: #0f172a;
	padding: 15px 20px;
	border-top-left-radius: 10px;
	border-top-right-radius: 10px;
	border-bottom: 2px solid #334155;
}

#header h1 {
	font-size: 20px;
	font-weight: bold;
	color: #38bdf8;
}

#close_btn {
	background: #ef4444;
	border: none;
	color: white;
	padding: 5px 10px;
	border-radius: 6px;
	font-weight: bold;
	cursor: pointer;
}

#close_btn:hover {
	background: #dc2626;
}

#content {
	padding: 25px;
}

p {
	margin-bottom: 20px;
	line-height: 1.5;
}

.form-group {
	margin-bottom: 25px;
}

.form-group label {
	display: block;
	margin-bottom: 8px;
	color: #94a3b8;
	font-weight: bold;
}

input[type="text"] {
	width: 100%;
	padding: 10px;
	background: #0f172a;
	border: 1px solid #334155;
	border-radius: 6px;
	color: white;
	box-sizing: border-box;
}

input[type="text"]:focus {
	border-color: #38bdf8;
}

button.primary {
	width: 100%;
	padding: 12px;
	background: #0284c7;
	border: none;
	border-radius: 6px;
	color: white;
	font-weight: bold;
	cursor: pointer;
}

button.primary:hover {
	background: #0369a1;
}

button.primary:active {
	background: #075985;
}
```

### Example Lua Mod (`mods/rmlui_example/init.lua`)
```lua
local ui = core.rmlui.create("rmlui_example")

-- Load and display the document
ui:load("mod://rmlui_example/ui/example.rml")
ui:show()

-- Register event callbacks
ui:on("click", function(id)
	core.log("action", "[RmlUi] User clicked element ID: " .. tostring(id))
	if id == "close_btn" then
		ui:close()
	elseif id == "submit_btn" then
		local name_input = ui:find("name_input")
		if name_input then
			core.log("action", "[RmlUi] Submitted input name: " .. name_input.id)
		end
	end
end)
```
