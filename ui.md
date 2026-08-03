# Next-Generation Formspec Architecture (Modern UI Framework)

Goal: Create a modern UI framework that replaces the limitations of traditional formspecs while remaining easy for mod developers to use. The system supports responsive layouts, reusable widgets, procedural animations, styling, custom canvas drawings, and native rounded-corner rendering in IrrlichtMt.

---

## 1. Introduction

The Next-Generation Formspec Architecture (or **Modern UI Framework**) introduces a declarative, object-oriented, and reactive UI framework. Instead of manually constructing complex, coordinate-based formspec strings, developers describe the interface hierarchy using standard Lua table structures, and the engine automatically handles layout calculations, styling, rendering, and reactivity.

### Example comparison

**Traditional Formspec:**
```lua
local spec = "size[8,6]" ..
             "button[1,1;2,1;play;Play]" ..
             "label[1,0.5;Welcome]"
minetest.show_formspec(playername, "main_menu", spec)
```

**Modern UI Framework:**
```lua
local ui_root = modern_ui.build(function()
    return window {
        width = 8,
        height = 6,
        column {
            spacing = 1,
            label { text = "Welcome" },
            button { id = "play", text = "Play" }
        }
    }
end)

modern_ui.show(playername, "main_menu", ui_root)
```

---

## 2. API Reference

### Global namespace: `modern_ui`

The global table `modern_ui` exposes the following core functions:

#### `modern_ui.build(func)`
Executes the provided function `func` inside a sandboxed environment where all declarative builders (such as `window`, `column`, `button`, etc.) are injected into the scope. This allows writing nested builders using clean, standard Lua syntax.

#### `modern_ui.show(playername, formname, widget_tree)`
Calculates the layout for the given `widget_tree` and renders it as an optimized formspec string to the specified player. It also sets up standard event routing and lifecycle hooks to clean up resources when the player closes the form or disconnects.

#### `modern_ui.reactive(initial_state)`
Creates a reactive state observer table. Any modifications to properties in this table will trigger automatic UI re-renders on all active forms subscribing to them.

#### `modern_ui.register_widget(name, constructor)`
Registers a custom widget class/subtree under the specified name. Registered custom widgets can be used directly inside sandboxed DSL scopes as builders.

#### `modern_ui.register_style(name, properties)`
Registers a centralized reusable style rule.

#### `modern_ui.register_theme(name, default_styles)`
Registers a centralized UI theme. Default themes include `"dark"`, `"light"`, and `"game"`.

#### `modern_ui.set_theme(name)`
Sets the active theme globally.

---

### Declarative Builders

The following builders are exposed within sandboxed DSL scopes:

| Builder | Type | Description |
|---|---|---|
| `window` | Layout | Root container for the UI |
| `panel` | Container | Solid background container with optional borders |
| `row` | Layout | Lays out children horizontally |
| `column` | Layout | Lays out children vertically |
| `grid` | Layout | Lays out children in rows and columns |
| `stack` | Layout | Overlays children on top of each other |
| `anchor` | Layout | Positions children relative to parent edges |
| `absolute` | Layout | Lays out children using manual coordinates |
| `label` | Display | Renders standard read-only text |
| `richtext` | Display | Renders hypertext (rich formatted text) |
| `image` | Display | Renders a textured image |
| `icon` | Display | Renders a sprite or icon |
| `scrollview` | Container | Scrollable container |
| `tabs` | Container | Tab-based container |
| `splitview` | Container | Side-by-side or top-and-bottom split panel |
| `button` | Input | Standard click button |
| `checkbox` | Input | Checkbox control |
| `toggle` | Input | Toggle switch control |
| `slider` | Input | Scrollbar/slider control |
| `switch` | Input | Toggling switch control |
| `canvas` | Advanced | Drawing surface for lines, rectangles, and custom shapes |
| `viewport` | Advanced | Embeds live 3D secondary camera rendering |
| `model_preview`| Advanced | Embeds a 3D model viewport |

---

## 3. Responsive Layout System

The framework automatically calculates coordinates based on constraints. Sizing properties can be:
- **Numbers:** Absolute grid units (e.g. `1.5`, `10`).
- **Percentages:** Strings like `"50%"` or `"100%"` relative to parent size.
- **`"auto"`:** Calculated automatically based on children or text size.

### Layout parameters

- `width` / `height`: Sets element dimensions.
- `min_width` / `max_width`: Sets constraints for auto-size limits.
- `padding`: Table `{ top, bottom, left, right }` or number.
- `margin`: Table `{ top, bottom, left, right }` or number.
- `spacing`: Grid/Column/Row spacing.
- `flex`: Flex factor for row/column proportions.

---

## 4. Styling & Native IrrlichtMt Rounded Corners

A widget can be styled using:
- Cascading theme defaults (defined per theme).
- Centralized reusable style classes (via `style = "primary_button"`).
- Inline overrides (via `styles = { background = "#FF0000" }`).
- Interactive state overrides (e.g. `hover`, `focused`, `disabled` styles nested inside properties).

### Native BORDER_RADIUS support

IrrlichtMt has been modified to support native rounded corners and border drawing in the rendering pipeline.

To draw rounded corners:
```lua
panel {
    styles = {
        background = "#3B82F6",
        radius = 12, -- Native C++ rounded corner pixel radius
        border_color = "#FFFFFF",
        border_width = 2
    }
}
```

---

## 5. Animation Engine

Every widget supports smooth, procedural animations utilizing standard easing functions:
- `linear`: Standard linear transition.
- `easeIn`: Eases in quadratically.
- `easeOut`: Eases out quadratically.
- `easeInOut`: Eases in and out quadratically.
- `elastic`: Elastic spring-like behavior.
- `bounce`: Clean bouncing transition.
- `spring`: Soft dampening spring transition.

### Animating a property

To animate any property of a widget (including translations, scale, rotation, and color transitions):
```lua
widget:animate(property_name, target_value, duration, easing_name, callback)
```

**Example (Spring translation offset on hover):**
```lua
local btn = button {
    text = "Play",
    on_hover = function(self)
        self:animate("x_offset", 1.5, 0.5, "spring")
    end
}
```

---

## 6. Reactive Data Binding

The reactivity system uses state observers to notify widgets of data changes and automatically re-render the UI:

```lua
local player_state = modern_ui.reactive({ health = 100 })

local ui = modern_ui.build(function()
    return window {
        label {
            text = player_state:bind("health", function(val)
                return "HP: " .. tostring(val)
            end)
        }
    }
end)

-- Updates automatically on player's screen!
player_state.health = 80
```

---

## 7. Custom Canvas drawings

The `canvas` widget allows custom vector and shape drawings:

```lua
canvas {
    width = 6,
    height = 6,
    draw_list = {
        { type = "rect", x = 1, y = 1, w = 4, h = 4, color = "#FF0000" },
        { type = "line", x1 = 0, y1 = 0, x2 = 6, y2 = 6, color = "#00FF00", thickness = 0.1 }
    }
}
```

---

## 8. Formspec Compatibility Parser

Traditional, legacy formspec strings continue working out of the box. They are parsed into the modern widget tree, allowing you to seamlessly integrate legacy mod interfaces with the next-generation renderer:

```lua
local parsed_tree = modern_ui.parse_compatibility_formspec("size[12,9] button[2,2;3,1;ok;Confirm]")
```
