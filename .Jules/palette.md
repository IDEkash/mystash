## 2026-07-10 - Modern Pause Menu Card Redesign
**Learning:** Modernizing in-game menus in Luanti is best done using formspec version 10, which allows absolute card-like grid layouts. We can use semi-transparent `box` overlays to act as visual container cards, and apply standardized action colors via `style` selectors (e.g. `#467832` for primary continue, `#43464b` for slate-gray settings, and dark red hues for exit actions). Stacking active buttons dynamically in C++ ensures robust responsive layouts.
**Action:** Always prefer containerized/card-like visual grouping and consistent button styling for modernized UI components.

## 2026-07-10 - Modern Main Menu Navigation and Segmented Control Redesign
**Learning:** Legacy native tabheaders can be completely replaced by overriding `tabview:tab_header(size)` in `builtin/fstk/tabview.lua` to generate custom button-based segmented tab controls. By using horizontal button tracks with solid card colors (`#25282c`), borderless styled buttons, and active/inactive state classes (`bgcolor=#467832` vs `#25282c`), we can achieve state-of-the-art visual elevation that feels highly responsive. This elevates the entire client interface to a pristine Material-like segmented pill standard.
**Action:** Overriding primitive layout generators is highly effective for global UI styling and visual modernization across all client tabs.
