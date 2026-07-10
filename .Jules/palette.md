## 2025-01-24 - [Visibility Perspective System]
**Learning:** When implementing visibility logic for objects that can have attachments, it's critical to separate the visibility of the root scene node (which must often remain "visible" in the engine to update and render children) from the visibility of the actual mesh data.
**Action:** Use `updateMeshCulling` (or similar per-material culling) to hide the local player's body in first-person while keeping the CAO node active so that attachments (like viewmodels) can still be rendered and positioned correctly.

## 2025-05-15 - Standardized Button Action Colors
**Learning:** Luanti's default UI lacks clear visual hierarchy for actions. Standardizing colors for primary, secondary, and destructive actions significantly improves scanability and prevents accidental deletions.
**Action:** Use #467832 (green) for primary "Go" actions, #43464b (dark grey) for "Back/Cancel" actions, and red for destructive actions, all with white text for high contrast.
