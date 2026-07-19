# External Logic Architecture (Draft v0.2)

> Status: Planning

External Logic is the developer-facing framework of the engine.

It contains everything created outside of the engine itself.

Examples:

- Mods
- Plugins
- Game Systems
- RPG Packs
- Horror Packs
- Vehicle Packs
- Survival Systems

Every External Logic package follows the same project structure.

```
MyProject/
│
├── Workspace/
├── Assets/
├── ClientSideService/
├── ServerSideService/
└── Storage/
```

---

# Philosophy

Each folder has **one responsibility only.**

| Folder | Responsibility |
|----------|----------------|
| Workspace | Definitions |
| Assets | Resources |
| ClientSideService | Client Logic |
| ServerSideService | Server Logic |
| Storage | Runtime Data |

Nothing should mix responsibilities.

---

# Workspace

Workspace is the **Definition Library**.

Workspace does NOT contain models.

Workspace does NOT contain textures.

Workspace does NOT contain sounds.

Instead, Workspace describes how objects are built and behave.

Think of it as blueprints.

The engine automatically scans this folder.

```
Workspace/
│
├── Blocks/
├── Characters/
├── Entities/
├── Items/
├── Weapons/
├── Tools/
├── Lights/
├── Effects/
├── Particles/
├── UI/
├── Materials/
├── Pathfinding/
├── Vehicles/
├── Structures/
└── ...
```

Nested folders are fully supported.

---

# Character Definition

```
Workspace/
└── Characters/
    └── Zombie.character
```

Example:

```ini
Name = "Zombie"

DisplayName = "Zombie"

Description = "Basic hostile monster."

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

Notice:

No Lua logic exists here.

Only definitions.

---

# Block Definition

```
Workspace/
└── Blocks/
    └── Stone.block
```

Example:

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

Example of a custom machine block:

```
Workspace/
└── Blocks/
    └── Machine.block
```

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

---

# Item Definition

```
Workspace/
└── Items/
    └── Sword.item
```

```ini
Name = "Iron Sword"

Model = "Models/Sword"

Texture = "Textures/Sword"

Damage = 25

Durability = 250

ServerScript = "Sword"
```

---

# Vehicle Definition

```
Workspace/
└── Vehicles/
    └── Car.vehicle
```

```ini
Name = "Car"

Model = "Models/Car"

Seats = 4

MaxSpeed = 80

ServerScript = "Car"

ClientScript = "Car"
```

---

# Effect Definition

```
Workspace/
└── Effects/
    └── Explosion.effect
```

```ini
Particle = "Explosion"

Sound = "Explosion"

Duration = 3
```

---

# Light Definition

```
Workspace/
└── Lights/
    └── Torch.light
```

```ini
Brightness = 12

Color = Orange

Range = 10

Shadow = true
```

---

# Pathfinding Definition

Pathfinding is reusable.

Many NPCs can share the same pathfinding profile.

```
Workspace/
└── Pathfinding/
    └── Monster.pathfinding
```

Example:

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

Character:

```
Pathfinding = "Monster"
```

---

# Assets

Assets only contain resources.

No gameplay logic.

```
Assets/
│
├── Models/
├── Textures/
├── Sounds/
├── Fonts/
├── Materials/
├── UI/
└── Videos/
```

Example:

```
Assets/
│
├── Models/
│   ├── Zombie.glb
│   ├── Sword.glb
│   ├── Car.glb
│   └── Cube.glb
│
├── Textures/
│   ├── Zombie.png
│   ├── Stone.png
│   ├── Machine.png
│   └── Sword.png
│
├── Sounds/
│   ├── Explosion.ogg
│   ├── Zombie.ogg
│   └── SwordHit.ogg
│
└── UI/
    ├── Inventory.html
    ├── Inventory.css
    └── Inventory.js
```

---

# ClientSideService

Contains client-only Lua scripts.

Responsible for visuals.

```
ClientSideService/
│
├── Camera.lua
├── UI.lua
├── Weather.lua
├── Lighting.lua
├── Fog.lua
└── Particles.lua
```

Examples:

- Camera
- HUD
- UI
- Fog
- Post Processing
- Local Animations
- Local Audio
- Input

---

# ServerSideService

Contains gameplay Lua scripts.

```
ServerSideService/
│
├── ZombieAI.lua
├── Machine.lua
├── Combat.lua
├── Inventory.lua
├── Quest.lua
└── Economy.lua
```

Examples:

- AI
- Combat
- Inventory
- Quests
- Saving
- World Events
- Economy
- Multiplayer

---

# Storage

Contains runtime generated files.

```
Storage/
│
├── Client/
└── Server/
```

Client

```
Storage/
└── Client/
    ├── Settings.json
    ├── Cache/
    └── Downloads/
```

Server

```
Storage/
└── Server/
    ├── World.db
    ├── Players.db
    ├── Economy.db
    └── Logs/
```

---

# Asset References

Definitions never reference file extensions.

Correct:

```
Model = "Models/Zombie"

Texture = "Textures/Zombie"

Sound = "Sounds/Explosion"
```

The engine resolves:

```
Models/Zombie.glb

Textures/Zombie.png

Sounds/Explosion.ogg
```

automatically.

---

# Automatic Registration

The engine automatically registers every definition.

```
Workspace/
    Characters/
        Zombie.character

↓

Engine

↓

Character Registered
```

No registration code.

No init.lua.

No boilerplate.

---

# Future Definition APIs (Planned)

Every definition may support the following sections.

```
Identity
Assets
Animations
Physics
Gameplay
Interaction
Navigation
Rendering
Networking
Components
Scripts
Audio
Events
Tags
Properties
Metadata
```

Examples:

```
Health
MaxHealth
WalkSpeed
JumpPower
Damage
Defense
CanCollide
CanTouch
CanInteract
Transparency
Visible
Color
Material
CollisionGroup
Mass
Gravity
Scale
Pivot
LOD
Shadow
Animator
RigidBody
AudioSource
LightSource
ParticleEmitter
Camera
Inventory
HealthComponent
```

---

# Overall Philosophy

- **Workspace** defines **what an object is**.
- **Assets** provide **the resources it uses**.
- **ClientSideService** controls **client-side behavior**.
- **ServerSideService** controls **gameplay and server logic**.
- **Storage** stores **runtime-generated data**.

The goal is to eliminate manual registration and make every game object declarative, reusable, and easy to organize. Future tools and editors can understand these definitions directly, enabling features like property editing, validation, and automatic asset linking without changing the underlying gameplay code.

# Structure Definition (.structure)

> Status: Draft v0.1

A `.structure` file defines a pre-built world structure that can be placed into the world by the engine.

Examples:

- House
- Village
- Dungeon
- Castle
- Tree
- Bridge
- Temple
- Ruins

Unlike blocks or characters, a structure stores an arrangement of blocks layer by layer.

The engine can place the structure anywhere in the world while automatically handling rotation, mirroring, and generation.

---

# Workspace

```
Workspace/
└── Structures/
    ├── House.structure
    ├── Village.structure
    ├── Castle.structure
    ├── Tree.structure
    └── Dungeon.structure
```

---

# Structure Layout

A structure is divided into multiple sections.

```
Structure
├── Metadata
├── Palette
├── Layers
├── Spawn Points
└── Settings
```

---

# Metadata

General information.

Example

```ini
Name = "Village House"

Author = "Developer"

Version = "1.0"

Category = "Village"

Description = "Basic village house."

Size = (7,5,7)

Origin = Center
```

---

# Palette

Instead of storing block names thousands of times, the structure stores a palette.

Example

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

The engine converts numbers into real block definitions.

Advantages

- Smaller files
- Faster loading
- Easier editing
- Better compression

---

# Layers

Blocks are stored layer-by-layer.

Each layer represents one Y level.

Example

```
Layer "Y0"
```

```
2 2 2 2 2 2 2
2 2 2 2 2 2 2
2 2 2 2 2 2 2
2 2 2 2 2 2 2
2 2 2 2 2 2 2
2 2 2 2 2 2 2
2 2 2 2 2 2 2
```

Next layer

```
Layer "Y1"
```

```
1 1 1 1 1 1 1
1 0 0 0 0 0 1
1 0 3 0 3 0 1
1 0 0 4 0 0 1
1 0 3 0 3 0 1
1 0 0 0 0 0 1
1 1 1 1 1 1 1
```

Another layer

```
Layer "Y2"
```

```
1 1 1 1 1 1 1
1 0 0 0 0 0 1
1 0 0 0 0 0 1
1 0 0 0 0 0 1
1 0 0 0 0 0 1
1 0 0 0 0 0 1
1 1 1 1 1 1 1
```

The engine stacks every layer vertically.

---

# Spawn Points

Optional.

Allows entities to spawn automatically after the structure is generated.

Example

```ini
SpawnPoints
{
    Villager = (3,1,2)

    Chest = (4,1,5)

    Zombie = (2,1,4)
}
```

---

# Markers

Optional.

Special positions used by gameplay.

Example

```ini
Markers
{
    FrontDoor = (3,1,0)

    Center = (3,1,3)

    Bed = (5,1,4)
}
```

Scripts can access these markers.

---

# Generation Settings

Optional.

Controls how the engine places the structure.

Example

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

---

# Random Variants

A structure can contain multiple variants.

Example

```
House.structure

Variant
├── Small
├── Medium
└── Large
```

The world generator can randomly choose one.

---

# Built-in API (Future)

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

# Automatic Registration

Every structure inside Workspace is registered automatically.

Example

```
Workspace/

Structures/

House.structure
```

↓

```
Engine
```

↓

```
House Registered
```

No manual registration required.

---

# File Philosophy

A `.structure` file should only describe the structure.

It should never contain Lua code.

Gameplay logic belongs inside:

- ClientSideService
- ServerSideService

---

# Future Optimizations

Although the `.structure` file is human-readable, the engine may automatically compile it into an optimized binary format during packaging.

Possible optimizations include:

- Palette compression
- Run-length encoding (RLE)
- Chunk compression
- Binary serialization
- Fast streaming

This allows developers to edit readable files while keeping loading times fast in released games.

---

# Design Goals

- Human-readable
- Easy to edit
- Compact through block palettes
- Supports automatic registration
- Supports random generation
- Supports rotation and mirroring
- Supports markers and spawn points
- Ready for future procedural generation
- Optimized for fast loading and world generation


---

# Rig Definition

A `.rig` file defines the physical body structure of an object.

Unlike a `.character`, which defines gameplay properties, a `.rig` defines how an object is built internally.

It is responsible for:

- Model
- Skeleton
- Bones
- Bone Hierarchy
- Collision Shape
- Hitboxes
- Attachments
- Default Bone Transforms
- Body Scale

A rig can be reused by multiple characters.

For example:

- Human.rig
- Zombie.rig
- Spider.rig

This prevents every character from redefining the same body structure.

---

# Workspace

```
Workspace/
└── Rigs/
    ├── Human.rig
    ├── Zombie.rig
    ├── Spider.rig
    └── Dragon.rig
```

---

# Character Usage

Characters reference a rig.

Example

```ini
Rig = "Zombie"
```

The engine loads the rig automatically.

---

# Example Rig

```
Workspace/
└── Rigs/
    └── Zombie.rig
```

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
```

---

# Bones

The rig defines every bone used by the model.

Example

```ini
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
```

These names are used by the engine animation system.

Lua APIs also reference these bone names.

---

# Default Bone Transforms

Every bone can have a default transform.

Example

```ini
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
```

These values are automatically restored when needed.

---

# Collision

A rig controls the object's collision.

Example

```ini
Collision
{
    Shape = Capsule

    Radius = 0.45

    Height = 1.8
}
```

Future versions may support:

- Capsule
- Box
- Sphere
- Convex Hull
- Mesh

---

# Attachments

Attachments define named mounting points.

Example

```ini
Attachments
{
    Head

    Body

    LeftHand

    RightHand

    LeftFoot

    RightFoot

    Back
}
```

These are used for:

- Weapons
- Hats
- Armor
- Tools
- Effects
- Cameras
- Lights

---

# Runtime Features

The engine allows scripts to modify a rig during gameplay.

Supported operations include:

- Change Model
- Change Collision Shape
- Move Bones
- Rotate Bones
- Scale Bones
- Hide Bones
- Show Bones
- Enable Physics
- Disable Physics
- Replace Rig

---

# Built-in APIs (Future)

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

# Automatic Registration

Every rig inside the Workspace is automatically registered.

Example

```
Workspace/

Rigs/

Human.rig
```

↓

```
Engine
```

↓

```
Human Rig Registered
```

No registration code.

No init.lua.

---

# File Philosophy

A `.rig` file only defines the body's physical structure.

It should never contain gameplay logic.

Gameplay belongs inside:

- Character Definitions
- ClientSideService
- ServerSideService

The rig acts as the reusable foundation for animation, physics, bone manipulation, attachments, and collision across all characters and entities.

# Configuration Files (Draft v0.1)

Every package type contains a configuration file located in its root directory.

Configuration files identify the package before the engine loads its contents.

They also provide metadata used by the engine, package manager, and future editors.

---

# Internal Logic Configuration

```
Internal Logic/
│
├── internal.conf
├── ServerSideService/
└── Assets/
```

The `internal.conf` file describes an Internal Logic package.

Example:

```ini
Name = "Default Internal Logic"

Description = "Default built-in world generation."

Version = "1.0"

Author = "Engine"

EngineVersion = "1.0"
```

---

# External Logic Configuration

```
External Logic/
│
├── external.conf
├── Workspace/
├── Assets/
├── ClientSideService/
├── ServerSideService/
└── Storage/
```

The `external.conf` file describes an External Logic package.

Example

```ini
Name = "Survival System"

Description = "Adds survival mechanics."

Version = "1.0"

Author = "Developer"

EngineVersion = "1.0"
```

---

# Library Configuration

```
MyLibrary/
│
├── library.conf
└── Library/
```

The `library.conf` file describes a library package.

A library is a collection of External Logic packages that can be distributed together.

Libraries do not directly contain gameplay.

Instead, they organize multiple External Logic packages into one collection.

Example

```ini
Name = "Official Library"

Description = "Collection of gameplay packages."

Version = "1.0"

Author = "Developer"

EngineVersion = "1.0"
```

---

# Library Structure

A library always contains a folder named `Library`.

```
MyLibrary/
│
├── library.conf
└── Library/
    ├── RPG/
    ├── Vehicles/
    ├── Horror/
    └── Survival/
```

Each folder inside `Library` is an independent External Logic package.

Example

```
Library/
│
├── RPG/
│   ├── external.conf
│   ├── Workspace/
│   ├── Assets/
│   ├── ClientSideService/
│   ├── ServerSideService/
│   └── Storage/
│
├── Horror/
│   ├── external.conf
│   ├── Workspace/
│   ├── Assets/
│   └── ...
│
└── Vehicles/
    ├── external.conf
    └── ...
```

Every package remains completely independent.

The library simply groups them together for easier installation and distribution.

---

# Automatic Loading

The engine loads packages in the following order.

```
Load library.conf

↓

Open Library/

↓

Scan every package

↓

Read external.conf

↓

Load External Logic
```

If a package is invalid, only that package fails to load.

The remaining packages continue loading normally.

---

# Design Goals

- Simple package metadata
- Automatic package discovery
- Independent External Logic packages
- Easy distribution
- Easy installation
- Future package manager support
- Compatible with automatic loading


Your previous foundation is not cooked. The review actually confirms the opposite: the core separation is good. The missing parts are mostly ecosystem rules (identity, dependency, loading, conflicts), not a bad architecture.

The important change you mentioned is:

External Logic replaces the old Mod system

Internal Logic becomes the built-in engine/game foundation

They are separated because they have different levels of access.


I would add these two documents:


---

External Logic System Specification (Draft v0.3)

# External Logic Architecture

> Status: Planning

External Logic is the replacement for the traditional Mod system.

It contains everything created outside the engine.

Examples:

- Gameplay systems
- Horror systems
- RPG systems
- Vehicle systems
- Weapons
- New entities
- New blocks
- New mechanics
- Custom worlds

External Logic behaves like plugins.

It can extend the engine without modifying the engine source code.

---

# Package Structure

Every External Logic package follows this structure.

MyPackage/ │ ├── external.conf │ ├── Workspace/ │ ├── Assets/ │ ├── ClientSideService/ │ ├── ServerSideService/ │ └── Storage/

---

# external.conf

The configuration file identifies the package.

Example:

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
}


---

Package Identity System

Every package has a unique ID.

Example:

horror.expansion
rpg.system
vehicle.framework

Objects automatically inherit the package namespace.

Example:

Package:

horror.expansion

Definition:

Zombie.character

Internal ID becomes:

horror.expansion:Zombie

This prevents conflicts.

Two packages can both have:

Sword.item

because internally they become:

rpg.weapon:Sword

fantasy.weapon:Sword


---

Asset Resolution

Assets also use namespaces.

Example:

Model =
horror.expansion:Models/Zombie

The engine resolves:

External Logic Package

↓

Assets

↓

Models/Zombie.glb

No ambiguity.


---

Dependency System

Packages can require other packages.

Example:

Dependencies
{
    CombatSystem >= 2.0

    RPGLibrary >= 1.5
}

Loading order:

Engine

↓

Internal Logic

↓

Libraries

↓

External Logic


---

Conflict Rules

If two packages modify the same object:

Priority:

Internal Logic
        |
        |
External Logic Override
        |
        |
Runtime Changes

External Logic cannot overwrite engine-critical systems.


---

Runtime Validation

When loading:

Engine checks:

Missing assets

Missing scripts

Invalid definitions

Dependency errors

Version mismatch


Example:

Zombie.character

Missing:

ServerScript ZombieAI.lua

Result:

Warning:
Zombie loaded without AI


---

External Logic Permissions

External Logic has controlled access.

Allowed:

Create objects

Add definitions

Add scripts

Add assets

Modify worlds


Restricted:

Replace engine renderer

Modify memory systems

Change core networking



---

Goal

External Logic turns the engine into a platform.

Developers create content without modifying the engine.

---

# Internal Logic Architecture Specification (Draft v0.1)

```md
# Internal Logic Architecture

> Status: Planning

Internal Logic is the built-in foundation of the engine.

It replaces the old concept of "game files".

Internal Logic defines how the world itself works.

Unlike External Logic, Internal Logic has deeper engine access.

---

# Purpose

Internal Logic controls:

- World generation
- Chunk system
- Default assets
- Default rules
- Engine-supported gameplay foundations

Examples:

- Default terrain
- Default blocks
- Default world rules
- Default physics rules

---

# Structure

InternalLogic/ │ ├── internal.conf │ ├── ServerSideService/ │ └── Assets/

---

# internal.conf

Example:

```ini
Name = "Default World"

ID = "core.default"

Version = "1.0"

EngineVersion = "1.0"

Description = "Default internal world system."


---

ServerSideService

Contains internal world systems.

ServerSideService/
│
├── MapGeneration/
│
├── ChunkSystem/
│
└── WorldRules/


---

Map Generation

Map generation uses:

.gen

files.

Example:

ServerSideService/

MapGeneration/

default.gen


---

.gen Definition

Controls how worlds generate.

Example:

WorldName = "Default"

Seed = Dynamic


Biomes
{
    Forest

    Desert

    Ocean

    Mountains
}


Structures
{
    Village

    Dungeon

    Ruins
}


Noise
{
    TerrainHeight

    Temperature

    Moisture
}


---

Chunk Definition

Chunk rules use:

.border

files.

Example:

ServerSideService/

ChunkSystem/

default.border

Controls:

Chunk size

Shape

Borders

Loading behavior


Example:

Size = (16,16,16)

Shape = Cube

LoadDistance = 8

Streaming = true


---

Assets

Internal assets contain default engine resources.

Assets/

Textures/

Sounds/

Materials/

Examples:

Textures/

Stone.png

Grass.png

Water.png


Sounds/

WalkStone.ogg

BreakStone.ogg


---

Internal vs External

Feature	Internal Logic	External Logic

Engine foundation	Yes	No
World generation	Yes	Optional
Core rules	Yes	No
New gameplay	Limited	Yes
Mods replacement	No	Yes
Deep access	Yes	Limited
User created	Rare	Yes



---

Loading Order

Engine Core

↓

Internal Logic

↓

Libraries

↓

External Logic

↓

World Runtime


---

Philosophy

Internal Logic defines:

"How the engine works."

External Logic defines:

"What developers create with it."

Internal Logic is the foundation.

External Logic is the expansion.

---

One thing I would **not** change: your `Workspace / Assets / ClientSideService / ServerSideService` separation. That part is actually the strongest idea. The missing piece was simply adding **identity + dependency + permission boundaries** so it can survive a large ecosystem.
