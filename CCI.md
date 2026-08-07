# Roblox-Style Instance & Workspace Engine

This document describes the design, API, and implementation of the **Roblox-Style Instance & Workspace Engine**—a modern, high-performance, fully moldable scene-tree and GUI layout system developed natively for Luanti as an alternative to both legacy formspec and standard entity models.

---

## 1. Overview & Architecture

To achieve absolute creative freedom, modern engines model their workspace as a unified hierarchical scene-graph (the Roblox `game` tree). In this tree, every entity, player, camera, container, or user interface element is an **Instance**:

*   **Global `game` Tree:** The root workspace is available globally as `game.Workspace`.
*   **Automatic Parent-Child Replication:** Setting an Instance’s `.Parent` property automatically handles detaching it from its old parent, registering it under its new parent, and shifting child lists dynamically.
*   **Dynamic Properties & Events:** Intercepting property writes triggers a `.Changed` event, allowing instances to synchronize their state automatically with client viewports or physical engine parameters.
*   **Real Custom CSS GUI Engine:** Hierarchical GUI components (`ScreenGui`, `Frame`, `TextLabel`, `ImageLabel`, `TextBox`) reside inside the player's `PlayerGui` folder. When modified, they dynamically recompile themselves into HTML5/CSS and render smoothly on the player's screen via an optimized client-bound WebView.

---

## 2. API Design & Specifications

### Instance Core Creation

#### `Instance.new(className, [parent])`
Instantiates a new Instance of type `className`.
*   `className`: String name of the class (`Folder`, `Part`, `Camera`, `BoneTransform`, `ScreenGui`, `Frame`, `TextLabel`, `ImageLabel`, `TextBox`).
*   `parent`: (Optional) Parent Instance to attach to immediately.
```lua
local folder = Instance.new("Folder", game.Workspace)
```

---

### Shared Instance Methods & Properties

#### `Instance.ClassName` (Read-only string)
The class name of the instance.

#### `Instance.Name` (String)
The name of the instance. Defaults to its `ClassName`.

#### `Instance.Parent` (Instance pointer)
The parent of the instance. Setting this automatically rebuilds child lookup trees.

#### `Instance:GetChildren() -> list`
Returns an array list of all child Instances attached to this object.

#### `Instance:FindFirstChild(name) -> Instance | nil`
Searches children and returns the first child whose `.Name` matches `name`.

#### `Instance:FindFirstChildOfClass(className) -> Instance | nil`
Searches children and returns the first child whose `.ClassName` matches `className`.

#### `Instance:Destroy()`
Recursively destroys the Instance and all of its nested children, removing all parent/child bindings and reclaiming memory cleanly.

#### `Instance.Changed(propertyName, newValue, oldValue)` (Event callback)
Fires whenever a property on the instance is written to.

---

## 3. Class Definitions & Bridging

### `Folder`
A generic organizational container.

### `Part`
A physical item or model in the workspace.
*   Properties: `Position`, `Rotation`, `Velocity`, `Size`.
*   `ObjectRef`: Can bind a native Luanti `ObjectRef` (player or entity). Property changes to `Position`, `Rotation`, or `Velocity` automatically propagate down to the native physics engine!

### `Camera`
Controls a player's camera viewport.
*   Properties: `FieldOfView`, `Mode`, `Smooth`, `Tilt`.
*   Propagates camera parameters directly to Luanti’s core viewport camera.

### `BoneTransform`
Procedural joint controller for skeletal mesh animations.
*   Properties: `BoneName`, `Position`, `Rotation`, `Scale`, `Visible`.
*   Propagates overrides to native independent bone transforms on the nearest parent `Part`.

### `ScreenGui` / `Frame` / `TextLabel` / `ImageLabel` / `TextBox`
Standard UI layout components.
*   Setting layout properties (`Position`, `Size`, `BackgroundColor`, `Text`, `TextColor`, etc.) automatically compiles the hierarchy into custom CSS layouts (flexbox, gradients, shadow borders) and pushes them to the player's screen at up to 60 FPS.
*   Inputs like typing or touching trigger corresponding `.Activated` or `.TextChanged` callbacks on the Lua-side Instance!

---

## 4. Modern Composition Examples

### 1. Roblox-Style Draggable Part Spawning
```lua
-- Create a physical part in the Workspace
local brick = Instance.new("Part", game.Workspace)
brick.Name = "GoldenBrick"
brick.Position = { x = 10, y = 5, z = -10 }

-- Listen for position updates
brick.Changed = function(key, val)
	if key == "Position" then
		print("Brick moved to: " .. val.x .. ", " .. val.y)
	end
end
```

### 2. A Real Custom CSS-Styled Login Screen GUI
```lua
local player_name = "singleplayer"
local player_folder = game.Players[player_name]

if player_folder then
	-- 1. Create a ScreenGui canvas inside the player's PlayerGui
	local screen = Instance.new("ScreenGui", player_folder.PlayerGui)
	screen.Name = "LoginScreen"

	-- 2. Create a centered background panel Frame
	local panel = Instance.new("Frame", screen)
	panel.Name = "MainPanel"
	panel.Position = "center"
	panel.Size = "400px, 250px"
	panel.BackgroundColor = "rgba(20, 24, 35, 0.9)"
	panel.BorderRadius = "16px"

	-- 3. Add a TextLabel header
	local title = Instance.new("TextLabel", panel)
	title.Position = "10px, 20px"
	title.Size = "380px, 40px"
	title.Text = "Welcome to Luanti Roblox"
	title.TextColor = "#ffffff"
	title.TextSize = "20px"

	-- 4. Add an action Button Frame
	local button = Instance.new("Frame", panel)
	button.Position = "100px, 150px"
	button.Size = "200px, 45px"
	button.BackgroundColor = "linear-gradient(180deg, #4d90fe, #357ae8)"
	button.BorderRadius = "8px"

	local btn_txt = Instance.new("TextLabel", button)
	btn_txt.Position = "0px, 0px"
	btn_txt.Size = "200px, 45px"
	btn_txt.Text = "Enter World"
	btn_txt.TextColor = "#ffffff"

	-- 5. Bind action event (.Activated)
	button.Activated = function(self)
		minetest.chat_send_player(player_name, "Entering the modern world...")
		screen:Destroy() -- Removes UI cleanly from client device and memory
	end
end
```
