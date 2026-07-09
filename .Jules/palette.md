## 2025-01-24 - [Visibility Perspective System]
**Learning:** When implementing visibility logic for objects that can have attachments, it's critical to separate the visibility of the root scene node (which must often remain "visible" in the engine to update and render children) from the visibility of the actual mesh data.
**Action:** Use `updateMeshCulling` (or similar per-material culling) to hide the local player's body in first-person while keeping the CAO node active so that attachments (like viewmodels) can still be rendered and positioned correctly.
