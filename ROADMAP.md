# ASCIIEngine roadmap

| Phase | Status | Scope |
| --- | --- | --- |
| 0. Repository audit | Complete | Reviewed generation, movement, rendering, lighting, resource lifetimes, caching, and build configuration. Findings are in ARCHITECTURE.md. |
| 1. World scale and independent rocks | Complete | Configurable scale, deterministic placement, multi-cell footprints, shared collision and occlusion, glyph surfaces, and projected-size LOD. Passed a MinGW build, CPU checks, and an OpenGL launch smoke test. |
| 2. Object elevation, projection, and depth | Complete | XYZ glyph positions, adjustable orthographic camera, inverse-projected lighting and visibility, and depth-ordered alpha compositing. Passed projection and placement checks plus an OpenGL launch smoke test. |
| 3. Continuous terrain | In progress | Scattered material detail, blade heights, and animated water are in place. Continuous terrain elevation, material fields, and shoreline detail are pending. |
| 4. Volumetric objects | In progress | Independent 3D trees and faceted rock surfaces are in place. Cliffs, additional plant shapes, and tree LOD are pending. |
| 5. Advanced lighting | Pending | Dynamic normal-based shading, height-aware shadows, ambient occlusion, and material response. |
| 6. Rendering optimization | Pending | Profile representative scenes, then add spatial indexing, streaming, asynchronous generation, static GPU data, and explicit memory budgets. Validate the 100k–500k glyph target and stress-test toward 1M. |
| 7. Characters and interaction | Pending | Detailed animated glyph characters, continuous movement, and world interactions. |
| 8. Atmosphere and polish | Pending | Fog, weather, particles, water reflections, bloom, and tone mapping. |
