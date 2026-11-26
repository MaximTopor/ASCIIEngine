# ASCIIEngine architecture

## Coordinates and world ownership

The world uses X and Y for the ground plane and positive Z for elevation. The camera rotates the ground axes by azimuth, foreshortens the view direction by the sine of elevation, and lifts elevated geometry by its cosine. Its orthographic projection, inverse at a known Z, and camera-space depth share the same basis. The default view is about 15° azimuth and 38° elevation. Left and Right orbit, Up and Down tilt, the mouse wheel zooms, and R resets zoom. WASD still moves on the world axes.

`WorldScale` configures logical cell dimensions and the player-height reference. The defaults are 24 × 28 world units per cell and a 28-unit player reference. Rock diameter is configured separately from the collision grid. Tree height ranges from 3.5 to 6.2 player heights. Grass and flowers use smaller glyphs and elevations tied to the same reference.

`World` owns the seeded FastNoiseLite terrain grid, rock and tree placements, and packed rock and trunk occupancy overlays. Jittered placement lattices operate in physical world units. Rock occupancy samples the same irregular contour used by visual generation at cell centers. Forest cells establish where trees can grow. Placed tree trunks block movement in their anchor cells, and players can walk beneath crowns. Terrain, rock, and trunk occupancy still share one ground-plane collision grid.

## Rendering and lighting

The GLFW window owns the OpenGL 4.5 context and initializes GLAD before render resources are created. Application member lifetimes release GPU resources before the window. CMake builds one C++20 executable and copies shaders and assets beside it.

`WorldRenderer` uses inverse-projected view bounds for terrain and elevated objects. A second test culls individual glyphs in screen space with a margin for glyph size and rotation. Ground detail is dispersed within cells using deterministic jitter and bilinearly blended terrain materials. Water retains animated ASCII ripples. Terrain itself is flat and its underlying material grid is still discrete.

Trees generate glyphs for roots, tapered trunks, branches, and layered foliage using 3D positions. This geometry is cached on first visibility. Rocks use a faceted procedural height field, finite-difference normals, crack bands, and moss variation. Three rock detail levels sample the same irregular contour with wider spacing at lower detail. Up to 256 rock surfaces are retained in the CPU cache. Rocks and trunks also submit dark ground-layer glyphs for contact shading.

Ground glyphs are submitted before objects. Vegetation, rocks, trees, the player, and legacy elevated glyphs are composited back to front in 4,096 stable depth buckets. Each glyph is a camera-facing alpha-blended quad with one depth for compositing. This preserves coverage between glyphs and leaves crossing quads and same-bucket overlaps approximate. The CPU submits one instanced glyph draw using the FreeType atlas.

The Light Map uses the camera's inverse ground-plane basis for occlusion rays. Elevated glyphs sample lighting at their projected ground positions. The occlusion texture combines terrain opacity, solid rock footprints, and partial tree trunk opacity. Day/night ambient light, soft torch shadows, and partial transmission use the existing Light Map. Shadow rays are planar and there is no height-aware self-shadowing.

## Performance

`RenderStats` uses nonblocking GPU queries. The renderer rebuilds the visible instance stream and uploads it each frame. Rock and tree geometry are generated on demand, with the tree cache allowed to grow to the finite tree population. Ground detail is regenerated from its deterministic samples during submission. The depth order uses linear-time bucket sorting.

In short stationary 1280 × 720 Debug smoke runs on Intel UHD Graphics, the previous opening view reported 139–144 FPS, 45,021 instances, and 3.09 MiB uploaded per frame. The 2.5D view after screen culling and bucket ordering reported 127–135 FPS, 28,470 instances, and 2.06 MiB uploaded per frame with the same three draw calls. The projection exposes a wider ground area, so these are scene measurements with different visible content. Interactive coverage and high-density stress testing are pending.
