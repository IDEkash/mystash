# Developer Guide: Redesigned Luanti Modding Foundation

Welcome to the comprehensive developer reference for the redesigned Luanti Mod Foundation. This guide details how the new Roblox-style object-oriented architecture, C++ native viewport block streaming, platform velocity inheritance, and Rojo-style folder compilation work, complete with concrete code and directory structure examples.

---

## Table of Contents
1. [Overview & Philosophy](#1-overview--philosophy)
2. [The Global DataModel (game) & Services](#2-the-global-datamodel-game--services)
3. [Instance Lifecycle & OOP API](#3-instance-lifecycle--oop-api)
4. [Universal Attributes & Tags](#4-universal-attributes--tags)
5. [Property Changed Signals & Events](#5-property-changed-signals--events)
6. [Specialized Class Walkthroughs](#6-specialized-class-walkthroughs)
   - [Folder](#folder)
   - [Part / Block (Physical Voxel Sync)](#part--block-physical-voxel-sync)
   - [ViewportFrame (3D Real-Time Block Streaming)](#viewportframe-3d-real-time-block-streaming)
   - [Sound (3D Positional Audio)](#sound-3d-positional-audio)
   - [Script (Sandboxed Execution)](#script-sandboxed-execution)
7. [Moving Platforms & Named Collision Shapes](#7-moving-platforms--named-collision-shapes)
8. [Rojo-Style Filesystem Compiler](#8-rojo-style-filesystem-compiler)

---

## 1. Overview & Philosophy

The redesigned foundation transitions Luanti from a flat voxel modding system to a **hierarchical, code-driven, object-oriented game engine**.

Instead of registering flat node definitions and entities, you compose the game state as a **tree of Instances** parented under the global root `game` (DataModel).
- Every asset, part, folder, and script is an `Instance` in this tree.
- Complex gameplay is created by **combining universal behaviors** (Attributes, Tags, and Event Connections) rather than writing rigid monolithic logic.

---

## 2. The Global DataModel (game) & Services

At the root of the hierarchy is the `game` object (a subclass of `Instance`). The game is divided into specialized **Services** created dynamically using `:GetService(name)`:

- **`game.Workspace`**: Root of all physical assets and Parts. Moving parts parented here automatically place nodes in the 3D world.
- **`game.ReplicatedStorage`**: A shared organizational folder for shared modules or variables.
- **`game.ServerScriptService`**: Contains running Server Scripts. Scripts placed here automatically compile and execute.
- **`game.Players`**: Manages player instances.

### Example: Querying and Using Services
```lua
-- Fetch the Workspace service
local Workspace = game:GetService("Workspace")

-- Fetch or create a custom service (e.g., ReplicatedStorage)
local ReplicatedStorage = game:GetService("ReplicatedStorage")

-- Print the children of game
for _, service in ipairs(game:GetChildren()) do
    print("Active Service: " .. service.Name)
end
```

---

## 3. Instance Lifecycle & OOP API

All objects inherit from the base `Instance` class.

### Method Reference
- `Instance.new(className, parent)`: Creates a new instance of a class and optionally parents it.
- `instance.Name`: Reads/writes the instance's name.
- `instance.Parent`: Reads/writes the parent instance. Parenting an instance adds it to the target's child list.
- `instance:GetChildren()`: Returns a shallow list of children.
- `instance:GetDescendants()`: Recursively returns all descendants in a flat list.
- `instance:FindFirstChild(name, recursive)`: Locates a child by its Name.
- `instance:FindFirstChildOfClass(className, recursive)`: Locates a child by its ClassName.
- `instance:Clone()`: Deep-duplicates the instance, properties, attributes, tags, and children.
- `instance:Destroy()`: Removes from parent, clears children recursively, destroys physical world nodes/sounds, and disconnects all active connections.

### Example: Managing Hierarchy
```lua
-- Create a parent Folder
local folder = Instance.new("Folder")
folder.Name = "MyFolder"
folder.Parent = game.Workspace

-- Create a child Part
local part = Instance.new("Part")
part.Name = "Brick"
part.Parent = folder

-- Find the child
local found = folder:FindFirstChild("Brick")
print(found.Name) -- Output: Brick

-- Clone the entire folder
local clone = folder:Clone()
clone.Name = "ClonedFolder"
clone.Parent = game.Workspace
```

---

## 4. Universal Attributes & Tags

Primitives that make any Instance extremely customizable.

### Attributes (Instance-specific Metadata)
- `instance:SetAttribute(name, value)`: Stores custom key-value metadata.
- `instance:GetAttribute(name)`: Retrieves the value.
- `instance.AttributeChanged:Connect(callback)`: Fires when an attribute changes.

```lua
local part = Instance.new("Part")
part:SetAttribute("Health", 100)

part.AttributeChanged:Connect(function(name, value)
    print("Attribute " .. name .. " changed to " .. tostring(value))
end)

part:SetAttribute("Health", 75) -- Output: Attribute Health changed to 75
```

### Tags (Categorization / Grouping)
- `instance:AddTag(tag)`: Registers a string tag.
- `instance:RemoveTag(tag)`: Unregisters a tag.
- `instance:HasTag(tag)`: Checks if the tag exists.

```lua
local part = Instance.new("Part")
part:AddTag("Interactable")
part:AddTag("Lava")

if part:HasTag("Lava") then
    print("This part burns!")
end
```

---

## 5. Property Changed Signals & Events

### Property Change Listeners
Allows scripts to observe when properties of an instance are modified:
- `instance:GetPropertyChangedSignal(property):Connect(callback)`

```lua
local part = Instance.new("Part", game.Workspace)

part:GetPropertyChangedSignal("Position"):Connect(function(new_pos)
    print("Brick moved to: " .. vector.to_string(new_pos))
end)

part.Position = vector.new(0, 10, 0) -- Output: Brick moved to: (0, 10, 0)
```

### Signals (Events) Framework
A high-performance custom connection framework:
```lua
local event = Signal.new()
local connection = event:Connect(function(arg1, arg2)
    print("Fired with: " .. tostring(arg1) .. ", " .. tostring(arg2))
end)

event:Fire("A", "B") -- Output: Fired with: A, B
connection:Disconnect() -- Stops listening
```

---

## 6. Specialized Class Walkthroughs

### Folder
An organizational container. It carries no physical properties but forms the structure of your game hierarchy.

---

### Part / Block (Physical Voxel Sync)
A physical 3D block.
- **Properties**: `Position` (vector), `Size` (vector), `BlockType` (node name string, e.g. `"default:stone"`), `Color` (string), `Anchored` (boolean).
- **Physical Syncing**: When nested under `game.Workspace`, the engine's Lua-C++ sync layer automatically sets the voxel block in the physical world. If moved or modified, the old block is cleaned up and placed at the new coordinate! If deleted, the block is set to `"air"`.

```lua
local brick = Instance.new("Part")
brick.BlockType = "default:brick"
brick.Position = vector.new(0, 5, 0)
brick.Parent = game.Workspace -- Places a brick block at (0, 5, 0) in the world!

brick.Position = vector.new(0, 6, 0) -- Clears (0, 5, 0) and places the block at (0, 6, 0)!
brick:Destroy()                      -- Removes the block (sets it to air)!
```

---

### ViewportFrame (3D Real-Time Block Streaming)
Renders a live secondary camera scene and **streams the live 3D feed directly onto a block's face as a dynamic texture**!
- **Properties**: `CameraPosition` (vector), `CameraDirection` (vector), `FOV` (number), `Width` / `Height` (integers), `FPS` (integer), `TargetBlock` (vector).
- **Behavior**: If `TargetBlock` is specified, the C++ engine renders the secondary view to a GPU RenderTargetTexture and injects it via our C++ `overrideTexture` pipeline. Any block face using `"viewport_camera_" .. name` dynamically streams this real-time render!

```lua
local monitor = Instance.new("ViewportFrame")
monitor.Name = "CCTV1"
monitor.CameraPosition = vector.new(0, 50, 0) -- Put the camera high up in the sky
monitor.CameraDirection = vector.new(0, -1, 0) -- Face straight down
monitor.TargetBlock = vector.new(0, 1, 0)      -- Streams the live camera feed onto the block at (0, 1, 0)!
monitor.Parent = game.Workspace
```

---

### Sound (3D Positional Audio)
Plays positional sound.
- **Properties**: `SoundName` (string), `Volume` (number), `Pitch` (number), `Looped` (boolean), `Playing` (boolean).
- **Positional 3D Sound**: If parented to a `Part` or a player, the sound is emitted in 3D relative to the parent's coordinates.

```lua
local alarm = Instance.new("Sound")
alarm.SoundName = "default:alarm"
alarm.Volume = 1.0
alarm.Looped = true

-- Emit sound in 3D from our brick's position!
alarm.Parent = game.Workspace.Brick
alarm.Playing = true -- Starts playing!
```

---

### Script (Sandboxed Execution)
Compiles and runs custom Lua code under a secure environment.
- **Properties**: `Source` (string).
- **Secure Blacklist Sandbox**: The execution environment blocks access to unsafe systems (`os.execute`, `os.remove`, `io` library, `require`, `loadfile`, etc.), preventing security exploits while keeping all Luanti APIs fully available. Accessing `_G` redirects to the sandboxed table itself, preventing global scope bypasses.

```lua
local script_inst = Instance.new("Script")
script_inst.Name = "MyGameController"
script_inst.Source = [[
    local Workspace = game:GetService("Workspace")
    local brick = Instance.new("Part", Workspace)
    brick.Position = vector.new(0, 10, 0)
    brick.BlockType = "default:stone"

    print("Hello from a secure sandboxed Script!")
]]
script_inst.Parent = game:GetService("ServerScriptService") -- Automatically executes!
```

---

## 7. Moving Platforms & Named Collision Shapes

Advanced physics features exposed on `ObjectRef` (players and entities).

### Moving Platform Velocity Inheritance
Enables players to stand on moving objects (elevators, boats) and inherit their motion.
- **Friction/Solid Check**: The velocity inheritance only activates if the standing entity has physical collision enabled (`isPhysical()` is true). If an entity is phasable, velocity inheritance is forced off!
- **Lua API**:
  ```lua
  -- Set platform properties
  player:set_platform_behavior({
      enabled = true,
      carry_rotation = true, -- Also rotate the player as the platform spins!
      friction_override = 2.0
  })
  ```

### Named Per-Part Collision Shapes
Allows defining multiple independent named collision parts on a model (like head, torso, arm) instead of a single box.
- **Lua API**:
  ```lua
  -- Define multiple hitboxes
  entity:set_collision_parts({
      { name = "Head", shape = "Sphere", radius = 0.3, offset = vector.new(0, 1.8, 0) },
      { name = "Torso", shape = "Box", size = vector.new(0.6, 0.9, 0.4), offset = vector.new(0, 0.9, 0) }
  })

  -- Handle partial hits (headshots!)
  core.register_on_part_collision(function(object, part_name, other_object)
      if part_name == "Head" then
          print("HEADSHOT!")
      end
  end)
  ```

---

## 8. Rojo-Style Filesystem Compiler

Instead of using a visual editor, developers organize their mods into direct filesystem folder trees under `Workspace/Explorer/`.

During startup, the engine recursively compiles this structure:
- **Folders** -> Compiled as `Folder` instances.
- **`.lua` Files** -> Compiled as `Script` instances (executing automatically if under `ServerScriptService`).
- **`.part` / `.viewport` / `.sound` / `.ini` Files** -> Parsed into specialized class instances with predefined properties.

### File Layout Example: `MyMod/`
```
MyMod/
├── mod.conf
├── init.lua
└── Workspace/
    └── Explorer/
        ├── ReplicatedStorage/
        │   └── Config.ini (Sets attributes/metadata)
        ├── ServerScriptService/
        │   └── MobSpawner.lua (Automatically executes on boot!)
        └── Workspace/
            ├── Part1.part (Voxel brick placed in world)
            └── MapCamera.viewport (CCTV camera projecting feed)
```

### Example `.part` file: `Workspace/Explorer/Workspace/Part1.part`
```ini
ClassName = Part
Name = LobbyBrick
Position = (0, 10, 0)
BlockType = default:stone
Color = #FF0000
```

By structuring files in this manner, you gain the benefits of Git, standard code editors, and continuous integration, combined with the modular power of a complete Roblox-style modding environment!
