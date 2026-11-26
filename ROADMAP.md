# ASCIIEngine roadmap

| Phase | Status | Scope |
| --- | --- | --- |
| 0. Repository audit | Complete | Reviewed generation, movement, rendering, lighting, resource lifetimes, caching, and build configuration. Findings are in ARCHITECTURE.md. |
| 1. World scale and independent rocks | Complete | Configurable scale, deterministic placement, multi-cell footprints, shared collision and occlusion, glyph surfaces, and projected-size LOD. Passed a MinGW build, CPU checks, and an OpenGL launch smoke test. |
| 2. Object elevation, projection, and depth | Complete | XYZ glyph positions, adjustable orthographic camera, inverse-projected lighting and visibility, and depth-ordered alpha compositing. Passed projection and placement checks plus an OpenGL launch smoke test. |
| 3. Continuous terrain | In progress | Woodland benchmark, varied glyph ground cover, a soil path, and an animated stream are in place. Continuous elevation and richer shoreline detail are pending. |
| 4. Volumetric objects | In progress | Reusable surface sampling and oak, birch, and pine generation with curved branches, roots, foliage, and three tree LODs are implemented. Cliffs, bushes, and ferns are pending. |
| 5. Advanced lighting | In progress | GPU surface lighting for trees, rocks and material samples uses sun, moon, hemispherical ambient and world-space torch lighting with roughness and reflectivity. Height-aware shadows and migration of legacy surfaces remain pending. |
| 6. Rendering optimization | In progress | World-space GPU caching, static cutout, full-frame profiling, and a shared cache byte budget are implemented. Larger stress tests and batching for zoomed-out procedural views are pending. |
| 7. Characters and interaction | In progress | Modular armored knight with held equipment, idle and walk cycles, facing, and continuous movement is in place. Foot planting and further interaction are pending. |
| 8. Atmosphere and polish | Pending | Fog, weather, particles, water reflections, bloom, and tone mapping. |

## Woodland benchmark verification

- MinGW Debug build and OpenGL 4.5 launch smoke tests passed in both modes.
- CPU checks covered deterministic placement, five benchmark trees, solid and decorative stone footprints, camera-relative axial and diagonal movement, movement timestep consistency, and valid spawn positions in both modes.
- Interactive visual and input review is pending.
- A stationary 1280 × 720 Debug smoke run on Intel UHD Graphics reported 55–65 FPS in the benchmark with about 60,740 instances and 4.87 MiB uploaded per frame. Procedural mode reported 128–138 FPS with about 19,773 instances and 1.58 MiB per frame. Both submitted five draw calls. The earlier flat-content workload and these scenes expose different geometry, so their frame rates do not isolate the cost of the new renderer.

## Surface sampler and tree verification

- MinGW Debug build and the CTest surface suite passed. It checks all seven primitives, arbitrary maps, deterministic jitter, normal and tangent geometry, and density behavior.
- OpenGL 4.5 launch smoke tests ran the benchmark in daylight and the procedural world at night with the default torch enabled. Interactive visual verification and a controlled comparison of lighting modes remain pending.
- During the last three reporting intervals of the stationary 1280 × 720 benchmark on Intel UHD Graphics, the engine reported 121–133 FPS, roughly 198,000 submitted glyphs, 1.23–1.44 ms sampled CPU render time and 5.86–6.71 ms sampled GPU time. The 192-entry cache used 18.75 MiB, with zero static uploads per sampled frame. Dynamic uploads were 0.11–0.12 MiB per frame. The last report showed one high-detail tree and four medium-detail trees.
- The procedural smoke view used 1,079 active cache entries, above the entry target because active buffers are retained. Its last two sample intervals reported 99–108 FPS and roughly 200,500 glyphs. Revisit per-object submission and batching before increasing overall forest density.

## Dynamic surface lighting verification

- MinGW Debug build and all three CTest tests passed, including an OpenGL 4.5 readback test for torch direction, visibility, directional diffuse, roughness, reflectivity, normal visualization and cutout. Both game modes launched and exited cleanly. Interactive artistic review is pending.
- In stationary 1280 × 720 runs on Intel UHD Graphics, excluding the first reporting interval, the benchmark went from 117–120 FPS to 107–113 FPS. Sampled CPU render time changed from 1.31–1.35 ms to 1.30–1.55 ms and GPU time from 6.39–7.56 ms to 7.47–8.55 ms. The two added material displays increased counts from about 216,235 to 216,865 instances and from 197 to 198 draws.
- Benchmark cache memory was 20.51 MiB before and 22.21 MiB after. Both warmed-up runs reported 0 MiB static uploads per frame. Dynamic upload moved from 0.12 to 0.13 MiB per frame. The new procedural run reported 144 FPS after warm-up with about 133,011 glyphs, 53 draws and 13.57 MiB of cache data. These are short stationary samples.
