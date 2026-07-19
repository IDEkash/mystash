# Luanti Unified Engine Architecture Specification

This document defines the core architecture of the engine's logical separation, world foundation, and content extension ecosystem.

---

# Core Philosophy: Single Responsibility

The engine separates all content, assets, and logic into distinct packages following a strict directory structure. Each directory has exactly **one responsibility**.

| Folder | Responsibility | Details |
| :--- | :--- | :--- |
| **Workspace/** | Definitions | Declarative blueprints and data definitions. No logic or asset files. |
| **Assets/** | Resources | Raw, compiled, or optimized media assets (models, textures, audio, UI layout, etc.). |
| **ClientSideService/** | Client Logic | Client-only Lua scripts responsible for local input, UI, rendering, and audio. |
| **ServerSideService/** | Server Logic | Server-side Lua scripts responsible for gameplay, state management, and authoritative logic. |
| **Storage/** | Runtime Data | Runtime-generated read/write data, local/cloud cache, settings, and database storage. |

Nothing must ever mix these responsibilities.

---

# Part I: Internal Logic Architecture (Draft v0.1)

> Status: Planning

**Internal Logic** is the built-in foundation of the engine. It replaces the old concept of "game files". Unlike External Logic, Internal Logic is a part of the engine itself and has deeper engine access.

Its sole purpose is to **generate and manage the world** with the minimum systems required for the engine to function. It is not intended for gameplay systems or user content packs.

## Folder Structure

```
Internal Logic/
│
├── internal.conf
│
├── ServerSideService/
│   ├── Mapgen.gen
│   └── Chunk.border
│
└── Assets/
    ├── Textures/
    │   ├── Missing.png
    │   ├── Unknown.png
    │   └── Default.png
    │
    ├── Sounds/
    │   ├── FootstepStone.ogg
    │   ├── FootstepGrass.ogg
    │   ├── DigStone.ogg
    │   ├── PlaceBlock.ogg
    │   └── BreakStone.ogg
    │
    └── Materials/
        ├── Stone.material
        ├── Wood.material
        ├── Grass.material
        └── Metal.material
```

## internal.conf

Every Internal Logic package must contain an `internal.conf` configuration file in its root directory. This identifies the package and details its metadata before the engine loads its contents.

```ini
Name = "Default World"
ID = "core.default"
Version = "1.0"
EngineVersion = "1.0"
Description = "Default internal world system."
```

## ServerSideService

Contains the definition files that describe how the engine itself should partition, generate, and organize the world.

### Map Generation Definition (`.gen`)

A `.gen` file defines how the world is generated. It describes terrain algorithms, height parameters, biome placement, and other structural systems.

**Location:** `Internal Logic/ServerSideService/Mapgen.gen`

```ini
Name = "Default"
Seed = Random
Chunk = "Default"
SeaLevel = 64
MinHeight = -64
MaxHeight = 320
TerrainGenerator = "Noise"
Biomes = true
Structures = true
Caves = true
Ores = true
Rivers = true
Lakes = true
```

*Responsibilities:* Controls world seed, terrain generation, height limits, biomes, rivers, lakes, caves, ores, and structure placement. It must never contain gameplay logic.

### Chunk Definition (`.border`)

A `.border` file defines how the world is divided into chunks. It controls chunk layout and streaming but does not generate terrain.

**Location:** `Internal Logic/ServerSideService/Chunk.border`

```ini
Name = "Default"
Shape = Cube
Size = (16,16,16)
Vertical = Infinite
Compression = Binary
Streaming = true
```

*Responsibilities:* Defines chunk shape (e.g., Cube, Hexagonal, Octree, Sparse Voxel), chunk dimensions, vertical limits, streaming/loading behavior, save formats, and compression rules.

### Relationship Between `.gen` and `.border`

The world generator references a chunk definition directly:

```
Mapgen.gen
    ↓ (Chunk = "Default")
Chunk.border
```
- The `.gen` file decides **what** is generated.
- The `.border` file decides **how the world is partitioned**.

## Assets & Materials

Internal Logic contains built-in resources used by the engine during world generation.

### Built-in Materials (`.material`)

Materials allow multiple blocks to share common physical and audio properties.

```ini
Name = "Stone"
FootstepSound = "Sounds/FootstepStone"
BreakSound = "Sounds/BreakStone"
DigSound = "Sounds/DigStone"
Friction = 0.6
Bounciness = 0.1
Hardness = 3.0
```

---

# Part II: External Logic Architecture (Draft v0.3)

> Status: Planning

**External Logic** is the replacement for the traditional Mod system. It contains everything created outside of the engine itself (e.g., game systems, vehicle packs, weapon systems, RPG frameworks, custom worlds).

External Logic behaves like modular plugins that extend the engine without modifying the core engine source code.

## Folder Structure

Every External Logic package follows a strict project structure.

```
MyPackage/
│
├── external.conf
│
├── Workspace/
│   ├── Blocks/
│   ├── Characters/
│   ├── Entities/
│   ├── Items/
│   ├── Weapons/
│   ├── Tools/
│   ├── Lights/
│   ├── Effects/
│   ├── Particles/
│   ├── UI/
│   ├── Materials/
│   ├── Pathfinding/
│   ├── Vehicles/
│   └── Structures/
│
├── Assets/
│   ├── Models/
│   ├── Textures/
│   ├── Sounds/
│   ├── Fonts/
│   ├── Materials/
│   ├── UI/
│   └── Videos/
│
├── ClientSideService/
│   ├── Camera.lua
│   └── UI.lua
│
├── ServerSideService/
│   └── ZombieAI.lua
│
└── Storage/
    ├── Client/
    └── Server/
```

## external.conf

The `external.conf` configuration file identifies the package and manages metadata and dependency listings.

```ini
Name = "Horror Expansion"
ID = "horror.expansion"
Version = "1.0.0"
Author = "Developer"
EngineVersion = "1.0"
Description = "Adds horror gameplay systems."

Dependencies
{
    BaseLibrary >= 1.0
    CombatSystem >= 2.0
}
```

## Package Identity System & Namespaces

Every package has a unique ID (e.g., `horror.expansion`, `rpg.system`, `vehicle.framework`). Objects automatically inherit the package namespace.

```
Package: horror.expansion
Definition File: Zombie.character
Internal ID: horror.expansion:Zombie
```

This prevents naming collisions across different packages. For instance, packages `rpg.weapon` and `fantasy.weapon` can both define a `Sword.item` without conflict, as they resolve internally to:
- `rpg.weapon:Sword`
- `fantasy.weapon:Sword`

## Asset Resolution with Namespaces

Assets also utilize namespaces to ensure correct, unambiguous asset references. Definitions never reference file extensions. The engine automatically resolves standard extensions.

```
Model = "horror.expansion:Models/Zombie"
Texture = "horror.expansion:Textures/Zombie"
Sound = "horror.expansion:Sounds/Explosion"
```

The engine resolves:
- `Models/Zombie` → `Assets/Models/Zombie.glb`
- `Textures/Zombie` → `Assets/Textures/Zombie.png`
- `Sounds/Explosion` → `Assets/Sounds/Explosion.ogg`

## Dependency System

Packages declare required dependencies in their `external.conf`. The engine loads dependencies recursively before loading dependent packages.

## Conflict Rules & Priority

If multiple packages attempt to modify or overwrite the same object, the following authority hierarchy is enforced:

```
Internal Logic (Engine Foundation)
        |
        ↓
External Logic Override (Packages/Mods)
        |
        ↓
Runtime Changes (Dynamic World State)
```

External Logic packages are strictly prohibited from overwriting engine-critical systems or modifying core memory and networking layers.

## Runtime Validation

During initialization and world-loading, the engine performs validations. It checks for:
- Missing assets or unresolved namespace paths.
- Missing scripts (e.g., missing a referenced `ServerScript`).
- Invalid definitions (malformed format or invalid property ranges).
- Dependency errors (missing packages or version mismatches).

**Example Validation Warning:**
If `Zombie.character` references a `ServerScript = "ZombieAI"` that is missing from `ServerSideService/`:
```
[Warning] Object 'horror.expansion:Zombie' loaded without AI (ServerScript 'ZombieAI' not found).
```

## External Logic Permissions

External Logic has strictly controlled, sandboxed access.

- **Allowed Access:** Create objects, add definitions, import assets, run custom client/server scripts, modify the active world space.
- **Restricted Access:** Cannot replace the engine renderer, cannot modify memory systems directly, and cannot access core networking interfaces.

---

# Part III: Workspace Definitions (Definitions Library)

Workspace is the **Definition Library** of blueprints. It describes how objects are built and behave, but contains absolutely no media resources (no models, textures, sounds, or Lua logic code).

The engine automatically scans and registers all files in these directories with zero boilerplate or manual code registration.

## Character Definition (`.character`)

Defines character statistics, rigging, animations, physics, and referenced scripts.

**File:** `Workspace/Characters/Zombie.character`

```ini
Name = "Zombie"
DisplayName = "Zombie"
Description = "Basic hostile monster."
Rig = "Zombie"
Model = "Models/Zombie"
Texture = "Textures/Zombie"

Animations
{
    Idle = "Idle"
    Walk = "Walk"
    Run = "Run"
    Attack = "Attack"
    Death = "Death"
}

Physics
{
    Collision = "Capsule"
    Mass = 80
}

Gameplay
{
    Health = 100
    WalkSpeed = 4
    Damage = 10
}

Pathfinding = "Monster"
ServerScript = "ZombieAI"
ClientScript = "ZombieClient"

Tags =
{
    Enemy
    Monster
}
```

## Block Definition (`.block`)

Defines voxel physical interactions, event hooks, and rendering models.

**File:** `Workspace/Blocks/Stone.block`

```ini
Name = "Stone"
Texture = "Textures/Stone"
Model = "Models/Cube"

Physics
{
    Collision = "Box"
}

Interaction
{
    Breakable = true
    Placeable = true
    Walkable = true
}

Events
{
    OnTouch
    OnLook
    OnPunch
    OnPlace
    OnBreak
    OnInteract
}

ServerScript = "Stone"

Tags =
{
    Stone
}
```

**File:** `Workspace/Blocks/Machine.block`

```ini
Name = "Machine"
Model = "Models/Machine"
Texture = "Textures/Machine"

Animations
{
    Idle = "Idle"
    Working = "Run"
}

Interaction
{
    CanInteract = true
}

ServerScript = "Machine"
ClientScript = "Machine"

Tags =
{
    Machine
}
```

## Item Definition (`.item`)

Defines inventory item properties and damage/durability parameters.

**File:** `Workspace/Items/Sword.item`

```ini
Name = "Iron Sword"
Model = "Models/Sword"
Texture = "Textures/Sword"
Damage = 25
Durability = 250
ServerScript = "Sword"
```

## Vehicle Definition (`.vehicle`)

Defines speed, seats, model, and logic scripts of moving vehicles.

**File:** `Workspace/Vehicles/Car.vehicle`

```ini
Name = "Car"
Model = "Models/Car"
Seats = 4
MaxSpeed = 80
ServerScript = "Car"
ClientScript = "Car"
```

## Effect Definition (`.effect`)

Defines particle and sound timings for special effects.

**File:** `Workspace/Effects/Explosion.effect`

```ini
Particle = "Explosion"
Sound = "Explosion"
Duration = 3
```

## Light Definition (`.light`)

Defines illumination parameters and shadow-casting capability.

**File:** `Workspace/Lights/Torch.light`

```ini
Brightness = 12
Color = Orange
Range = 10
Shadow = true
```

## Pathfinding Definition (`.pathfinding`)

Pathfinding profiles are highly reusable. Multiple characters can share a single navigation profile.

**File:** `Workspace/Pathfinding/Monster.pathfinding`

```ini
CanJump = true
CanSwim = false
CanClimb = false
CanOpenDoors = false
JumpHeight = 1.2
SearchDistance = 64
UpdateRate = 0.3

Costs
{
    Grass = 1
    Water = 30
    Lava = 999
}
```

---

## Structure Definition (`.structure`)

A `.structure` file defines a pre-built voxel layout (e.g., house, dungeon, bridge) that the engine can generate, rotate, mirror, and place layer-by-layer.

**File:** `Workspace/Structures/House.structure`

### 1. Metadata
```ini
Name = "Village House"
Author = "Developer"
Version = "1.0"
Category = "Village"
Description = "Basic village house."
Size = (7,5,7)
Origin = Center
```

### 2. Palette
Instead of replicating block names redundantly, a palette maps ID values to block definitions.
```ini
Palette
{
    0 = Air
    1 = OakPlanks
    2 = Cobblestone
    3 = Glass
    4 = Door
    5 = Torch
    6 = Roof
}
```

### 3. Layers
Layout is parsed Y-level by Y-level, from bottom to top.
```
Layer "Y0"
2 2 2 2 2 2 2
2 2 2 2 2 2 2
2 2 2 2 2 2 2
2 2 2 2 2 2 2
2 2 2 2 2 2 2
2 2 2 2 2 2 2
2 2 2 2 2 2 2

Layer "Y1"
1 1 1 1 1 1 1
1 0 0 0 0 0 1
1 0 3 0 3 0 1
1 0 0 4 0 0 1
1 0 3 0 3 0 1
1 0 0 0 0 0 1
1 1 1 1 1 1 1

Layer "Y2"
1 1 1 1 1 1 1
1 0 0 0 0 0 1
1 0 0 0 0 0 1
1 0 0 0 0 0 1
1 0 0 0 0 0 1
1 0 0 0 0 0 1
1 1 1 1 1 1 1
```

### 4. Spawn Points & Markers (Optional)
```ini
SpawnPoints
{
    Villager = (3,1,2)
    Chest = (4,1,5)
    Zombie = (2,1,4)
}

Markers
{
    FrontDoor = (3,1,0)
    Center = (3,1,3)
    Bed = (5,1,4)
}
```

### 5. Generation Settings
```ini
Settings
{
    Rotation = Random
    Mirror = false
    AllowTerrainMerge = true
    ReplaceAirOnly = false
    Foundation = true
    SpawnChance = 0.25
}
```

### Built-in Structure APIs
```lua
Structure:Place(position)
Structure:Rotate(90)
Structure:Mirror()
Structure:GetMarker("Center")
Structure:GetSize()
Structure:Clone()
Structure:Destroy()
```

---

## Rig Definition (`.rig`)

A `.rig` file defines the internal physical assembly, skeleton, bones, hitboxes, attachments, and collision shapes of an entity. Rigs are reusable across different character definitions.

**File:** `Workspace/Rigs/Zombie.rig`

```ini
Name = "Zombie"
Model = "Models/Zombie"
CollisionShape = "Capsule"
Scale = 1.0
Hitbox = "Humanoid"

Attachments
{
    Head = "Head"
    RightHand = "RightHand"
    LeftHand = "LeftHand"
    Body = "Chest"
}

Bones
{
    Root
    Spine
    Chest
    Neck
    Head
    LeftShoulder
    LeftArm
    LeftHand
    RightShoulder
    RightArm
    RightHand
    LeftLeg
    LeftFoot
    RightLeg
    RightFoot
}

Transforms
{
    Head
    {
        Position = (0,0,0)
        Rotation = (0,0,0)
        Scale = (1,1,1)
    }
    LeftHand
    {
        Position = (0,0,0)
        Rotation = (0,0,0)
        Scale = (1,1,1)
    }
}

Collision
{
    Shape = Capsule
    Radius = 0.45
    Height = 1.8
}
```

### Built-in Rig APIs
```lua
Rig:SetModel()
Rig:GetModel()
Rig:SetCollisionShape()
Rig:GetCollisionShape()
Rig:SetBonePosition()
Rig:SetBoneRotation()
Rig:SetBoneScale()
Rig:GetBone()
Rig:HideBone()
Rig:ShowBone()
Rig:AddAttachment()
Rig:RemoveAttachment()
Rig:ResetBone()
Rig:ResetPose()
```

---

# Part IV: Package Distribution & Library Structures

The engine supports grouping multiple independent External Logic packages into a unified **Library Package** for easy installation and distribution.

## Library Configuration

A library package contains a `library.conf` configuration file in its root directory and groups all packages inside a directory named `Library/`.

```
MyLibrary/
│
├── library.conf
│
└── Library/
    ├── RPG/
    │   ├── external.conf
    │   ├── Workspace/
    │   ├── Assets/
    │   └── ...
    │
    ├── Horror/
    │   ├── external.conf
    │   ├── Workspace/
    │   └── ...
    │
    └── Vehicles/
        └── external.conf
```

**library.conf Example:**
```ini
Name = "Official Library"
Description = "Collection of gameplay packages."
Version = "1.0"
Author = "Developer"
EngineVersion = "1.0"
```

Each folder inside the `Library/` directory is treated as a fully isolated, independent External Logic package with its own `external.conf` and strict separation rules. If a single package in a library fails validation, the engine skips only that package and successfully loads the others.

---

# Part V: Internal Logic vs External Logic Comparison

| Feature | Internal Logic | External Logic |
| :--- | :--- | :--- |
| **Engine Foundation** | Yes | No |
| **World Generation** | Yes | Optional |
| **Core World Rules** | Yes | No |
| **New Gameplay Systems** | Limited | Yes |
| **Mod System Replacement** | No | Yes |
| **Deep Engine Access** | Yes | Limited (Sandboxed) |
| **User/Developer Created** | Rare | Yes |

---

# Part VI: Boot & Loading Sequence

The engine bootstrapper discovers and loads definitions, code, and resources in a strict, sequential order to prevent loading and dependency conflicts:

```
1. Engine Core Startup
   ↓
2. Load Internal Logic Packages (e.g., Read internal.conf, load Chunk.border & Mapgen.gen)
   ↓
3. Discover & Load Library Packages (e.g., Read library.conf, scan Library/ folder)
   ↓
4. Discover & Load External Logic Packages (e.g., Read external.conf, resolve Dependency trees)
   ↓
5. Run Asset & Workspace Validation (Verify IDs, asset namespaces, scripts, and integrity)
   ↓
6. Initialize World Runtime & Execute Lua Services (ClientSideService & ServerSideService)
```

---

# Part VII: Future Definition APIs (Planned)

Every declarative blueprint definition in the Workspace can support standardized block sections to map seamlessly to future editors and engine-level systems:

```
Identity      Assets       Animations    Physics       Gameplay
Interaction   Navigation   Rendering     Networking    Components
Scripts       Audio        Events        Tags          Properties
Metadata
```

This decoupling allows developers, graphic designers, and level editors to build, preview, and adjust worlds and gameplay entities inside graphical user interfaces without editing any Lua scripts directly.
