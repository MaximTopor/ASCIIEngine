# ASCIIEngine architecture

## Gameplay and benchmark

Run `ASCIIEngine.exe` from the build output directory for the deterministic woodland benchmark. Run `ASCIIEngine.exe --procedural` for the seeded 500 × 500 world. Both use the same movement, collisions, lighting, and rendering pipeline. The benchmark starts in daylight. Its seed, path and stream profiles, tree placements, and rock sizes are in `World::generateBenchmark`.

| Control | Action |
| --- | --- |
| WASD | Continuous camera-relative movement, including normalized diagonal movement |
| Mouse wheel / R | Zoom / reset to gameplay zoom |
| F3 | Toggle developer orbit and tilt. Leaving developer mode restores the fixed gameplay angles. |
| Left / Right, Up / Down | Orbit and tilt with developer mode active |
| F4 / F5 | Cycle lighting views / return to final lighting |
| L / P | Toggle the player torch / pause or resume the day/night cycle |
| 1 / 2 / 3 / 4 | Noon / sunset / midnight / sunrise |
| Escape | Exit |

X and Y form the ground plane. Positive Z is elevation. Camera projection, inverse projection, and depth use the same basis. The gameplay camera uses 0.37 rad azimuth, 0.73 rad elevation, and 1.45× zoom. Exponential follow uses frame delta time. Input is projected back onto the ground plane and passed to the existing continuous collision solver with substeps and axis sliding. Camera controls and projection defaults are in `Application.hpp/.cpp` and `Camera.cpp`.

## Scene and scale

The app configures a 64-unit knight in both modes. Benchmark collision cells are 16 × 16 units. Procedural mode keeps its 24 × 28 cell dimensions. Tiles supply terrain categories and the collision grid. Rock and trunk occupancy are independent overlays. Decorative stones are walkable. Rock footprints sample their physical contour, and trunks mark their base cells according to trunk radius. Crowns do not block ground movement. Water remains a traversal obstacle.

The benchmark has a meandering stream, shallows, sand banks, a soil path, flowers, leaf litter, mossy stone, and seed-controlled grass. Terrain glyph colors are interpolated between neighboring tiles and their positions are jittered. `WorldRenderer.cpp` controls those palettes and ground sample counts. `VegetationGenerator.cpp` controls blade height, density, plant palettes, and wind. Terrain is currently flat.

### Surface sampling and trees

`SurfaceSampler.hpp/.cpp` emits deterministic CPU samples with positions, normals, tangents, normalized surface coordinates, spacing for glyph sizing, and seed variation. It supports cylinders, tapered frusta, spheres, ellipsoids, capsules, planes, curved tubes, and caller-defined parametric maps. Curved tubes approximate arc length along a spline, transport ring frames along it, and stagger circumferential samples. An optional radial profile adds coherent cross-section variation and contributes to normals. Cylinders and frusta sample the lateral wall. Capsules include rounded ends. Generic parametric coverage depends on the physical lengths supplied by the caller and the distortion of its map.

`SurfaceMaterial.hpp/.cpp` maps samples to ASCII with configurable palettes, ink coverage, density, rotation and pigment variation. It passes the sample's world-space normal, roughness, reflectivity, and local ambient exposure to the glyph. Tree material settings sit near the top of `TreeRenderer.cpp`. `RockRenderer.cpp` retains its structural and decorative layers. Cracks and moss tint the albedo, and moss also interpolates roughness and reflectivity. Rock normals still come from the deformed surface. Leaf cluster normals include their existing radial deformation.

`TreeRenderer` builds a tapered spline trunk, curved roots that enter the ground, and anchored primary branches, secondary branches, and twigs. Ellipsoid shells cluster foliage around the branch tips. Coherent patch thinning creates openings, and inner foliage is darker. The oak has a wide crown and thick base, the birch has a narrower silhouette and pale marked bark, and the pine has compact dark needle clusters along a tapered crown. `World.cpp` sets the benchmark species and dimensions. `World.hpp` shares per-species trunk radius ratios with the collision footprint.

High, medium, and low tree LOD use 4.7, 7.6, and 12.0 world-unit base sample spacing. Projected thresholds are 520 and 225 pixels with 12% hysteresis; the size estimate accounts for crown width at steep camera angles. The layout stays deterministic across LOD levels, and the low level omits twigs. The first encounter with an uncached tree/LOD builds and uploads it directly. Warm frames reuse the GPU buffer without storing a second CPU copy of the tree. WorldRenderer still handles visibility, iteration, and contact shadows.

## Knight

`KnightRenderer` owns a parent-ordered hierarchy of rigid body parts. Arms and legs have independent articulated segments. The sword and shield attach to the hands. Part-local ellipsoid surfaces are sampled in offset rings, and each sample records its normal, material and glyph size. Joint offsets, armor proportions, palette, sample spacing, and glyph ink sizing are in `KnightRenderer.cpp`. The model includes a closed helmet with visor, pauldrons, breastplate, gauntlets, boots, belt, shield, and sword. At very small projected sizes the knight uses a reduced sample stride.

Breathing animates the chest at rest. Actual movement distance advances the walking cycle, and actual displacement controls facing. Parent transforms drive stride, knees, ankle angles, torso lean, arm swing, and equipment. The walk uses procedural joint angles and ground clamping. It does not use foot planting or inverse kinematics.

The knight has separate steel, polished edge, recess, cloth, leather, and shield materials. World-space normals receive day-dependent diffuse and specular shading before the shared Light Map modulates the result. The torch and ambient light affect the entire scene. Ground-layer glyphs approximate contacts. Shadow rays in the Light Map remain planar.

## Glyph pipeline and performance

The GLFW window owns the OpenGL context. GLAD loads before GPU resources are created, and application member lifetimes release those resources before the window. The existing FreeType atlas contains printable ASCII with padding aligned to the two limited mip levels. Mip filtering is used on the atlas. The occlusion map keeps its original filtering and lookup.

The frame renders the Light Map and fullscreen background, then uses instanced glyph passes for cached ground, dynamic ground and shadows, static surface cutouts, and the dynamic core and edge passes.

Static vegetation, rocks, and trees store world-space instance data in immutable GPU buffers. GLSL handles projection, per-glyph depth and wind. They use the existing alpha cutout thresholds and shared depth buffer. Dense and narrow glyph palettes are tuned separately while the global cutout threshold stays the same. Dynamic glyphs retain the existing opaque-core and depth-sorted antialiased passes.

The shared cache has a 512-entry target and a 96 MiB instance-buffer budget in `GlyphRenderer.hpp`. Least recently used buffers from inactive frames are eligible for eviction. Active geometry can temporarily exceed either target when the view requires it. Tree keys include the tree ID and LOD and use a separate key range from rocks, vegetation and ground.

`RenderStats` reports upload bytes, instances, draw counts and asynchronous GPU time. The cache report adds entries, allocated instance bytes, last-frame hits and misses, and visible tree counts by LOD. `CpuProfiler` includes event polling, input, world lighting, player and camera updates, rendering, swap, and the total measured frame interval. Rendering includes the listed render substages, so their averages should be read as a breakdown. See ROADMAP.md for measurements and remaining checks.

## Dynamic surface lighting

`VisualGlyph::color` is albedo for normal-enabled glyphs. `GlyphMaterial` stores a world-space normal, roughness, reflectivity, ambient occlusion and a lighting mode. Legacy mode is the default. Tree wood, foliage, both rock layers and the two benchmark material samples use surface lighting. Ground, grass, water, the knight and other existing glyphs retain their compatible Light Map path. The knight keeps its existing shading.

`GlyphInstance` grew from 100 to 108 bytes. Locations 0–9 retain their existing layouts. Integer attribute 10 reads two unsigned words at byte offsets 100 and 104. The first packs three 10-bit normal components and two mode bits. The second stores 8-bit roughness, reflectivity and ambient occlusion with one reserved byte. The vertex shader renormalizes decoded normals. Compile-time assertions validate the stride and new offsets, and the renderer checks the context has at least 11 attributes. Both static and dynamic instance buffers use the same layout. Cache memory accounting uses the actual stride.

`Lighting.cpp` controls directional azimuth, elevation, color curves and hemispherical sky/ground ambient. Sun and moon directions and radiance change with the existing clock. The vertex shader evaluates diffuse and Blinn–Phong specular at the glyph anchor, using the orthographic camera basis for view direction. Roughness controls highlight width and intensity. Reflectivity scales the response. Local ambient occlusion affects ambient only. Foliage receives wrapped diffuse and some backlighting while its billboard wind animation continues on the GPU.

`Application.cpp` configures the world-space torch height, range, source radius, intensity and flicker. The shader supports four world-space point lights with bounded quadratic falloff. The primary light uses the Light Map alpha channel for visibility. Additional surface lights currently use unshadowed direct lighting. The existing planar shadow rays populate alpha, and RGB retains the previous combined ambient and point-light result for legacy glyphs and the background. Surface glyphs use alpha on the primary point contribution only. Offscreen ground projections use full visibility. Directional and point shadowing remain independent; directional shadows and height-aware point occlusion are future work.

The shader changes RGB while the existing coverage passes and depth rules remain intact. Light and camera updates change uniforms. LOD or visibility changes can bring a different cached buffer into view, or create it on its first encounter. The light map is now RGBA16F, adding two bytes per quarter-resolution texel. At 1280 × 720 that adds about 0.11 MiB.

F4 cycles Final, Albedo, Normals, Ambient, Directional diffuse, Point light, Specular, Torch visibility, Roughness, Reflectivity and Legacy light map. F5 restores Final. The benchmark places matched steel and matte stone samples southwest of the starting position in world coordinates, at offsets X −82 and −42 and Y +70. They share the same albedo and geometry so the material response is easier to compare. Use the roughness and reflectivity presets in `Lighting.hpp` for new surfaces. Tree and rock preset values can be tuned in their renderers.
