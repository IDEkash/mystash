# CCI UI System — Design & API Specification

CCI is a retained-mode 2D UI system for Luanti. It allows server-side mods to register scalable, styled vector UI definitions (styles) and display them on-screen for players as dynamic UI elements (instances) without using Formspecs.

The system enforces a clean separation of:
1. **Style** — defines what a UI object looks like (geometry + appearance).
2. **Geometry** — local shape defined by a set of coordinates (points + connections).
3. **Appearance** — controls filled coloring, opacity, and image rendering.
4. **Instance** — a rendering instance of a registered style.
5. **Position** — where on screen the instance appears.
6. **Layer** — determines rendering priority and order.

---

## 1. Style Definition

A style defines the shape and appearance of a UI element. It does not have any screen position and is registered globally on the server.

### `ui.style(style_name, def)`

- `style_name`: string (unique identifier for this style)
- `def`: table containing `geometry` and `appearance` sub-tables.

#### Geometry

Geometry defines the local boundary of the shape.
- `points`: key-value table mapping point names to local coordinates `{x, y}` (not screen coordinates).
- `shape`: list of connections between points.
  - Format: `{"POINT_A", "POINT_B", bend}` (where `bend` is an optional float). A bend of `0` is a straight line, while a non-zero value creates a quadratic Bezier curve.
  - Closed shapes: if connections form a closed loop, the renderer fills the enclosed region.

#### Appearance

Appearance controls how the geometry is filled and overlayed with images.
- `fill`: string (Hex ColorSpec, e.g. `"#EF4444"`)
- `opacity`: float (`0.0` to `1.0`)
- `image` (optional): table containing:
  - `[1]`: string texture path (e.g. `"textures/car.png"`)
  - `position`: local coordinates `{x, y}` relative to the object's geometry
  - `size`: float scale factor (defaults to `1.0`)

### Example:
```lua
ui.style("car", {
    geometry = {
        points = {
            a = {1, 3},
            b = {2, 2},
            c = {4, 2},
            d = {5, 3},
            e = {6, 3},
            f = {6, 4},
            g = {1, 4}
        },

        shape = {
            {"a", "b", 0.15},
            {"b", "c", 0.10},
            {"c", "d", 0.15},
            {"d", "e", 0.10},
            {"e", "f", 0.15},
            {"f", "g", 0.10},
            {"g", "a", 0.15}
        }
    },

    appearance = {
        fill = "#EF4444",
        opacity = 1.0,

        image = {
            "textures/car.png",
            position = {0, 0},
            size = 1.0
        }
    }
})
```

---

## 2. Creating an Instance

Once a style exists, you can instantiate it at any screen position.

### `ui.create(instance_name, style_name, properties)`

- `instance_name`: string (unique identifier for the created instance)
- `style_name`: string (name of the style to instantiate)
- `properties`: table containing:
  - `position`: table/vector representing the screen coordinate `{x, y}` where the instance should be drawn (automatically scaled by display density).
  - `layer`: integer (1 to 5).
    - `1 = highest` (drawn last, i.e., rendered on top of everything).
    - `5 = lowest` (drawn first, i.e., rendered in the background).
  - `player`: optional string (player name) or player `ObjectRef`. If provided, the UI instance will only be synchronized and shown to that specific player. Otherwise, it will be global for all players.

### Example:
```lua
ui.create("car_1", "car", {
    position = {100, 150},
    layer = 2,
    player = "singleplayer" -- or minetest.get_player_by_name("singleplayer")
})
```

---

## 3. Destroying an Instance

Active UI instances can be removed dynamically from the screen.

### `ui.destroy(instance_name, player)`
### `ui.delete(instance_name, player)` (Alias)

- `instance_name`: string (name of the instance to destroy)
- `player` (optional): string (player name) or player `ObjectRef`. If the instance was player-specific, provide the player to destroy it for that player.

### Example:
```lua
ui.destroy("car_1", "singleplayer")
```

---

## 4. Architecture Implementation Details

The system follows a retained-mode model:
- The Style Registry stores the immutable geometry and appearance of styles.
- When `ui.create` is invoked, the server instantiates a UI instance, assigning it a `position`, `layer`, and `player` filter.
- These definitions are synchronized to the client using new client-server network opcodes:
  - `TOCLIENT_CCI_STYLE` (0x69)
  - `TOCLIENT_CCI_CREATE` (0x6a)
  - `TOCLIENT_CCI_DESTROY` (0x6b)
- The Client-side Renderer retrieves active instances, sorts them based on Layer (Layer 5 first, down to Layer 1 last) and secondary creation order, transforms local coordinates into screen pixels dynamically scaled by high-DPI `m_scale_factor`, and renders them using standard Irrlicht 2D APIs.
- Curved connection segments defined with a non-zero `bend` are generated dynamically using quadratic Bezier curve formulas:
  $$P_c = Midpoint(P_0, P_1) + PerpendicularNormal(P_1 - P_0) \times bend$$
