# ASCIIEngine project rules

- Compose all visible world geometry from ASCII glyphs. Font atlases and other technical textures may support rendering.
- Keep physical object dimensions, visual glyph density, logical cell dimensions, and camera zoom independent.
- Preserve working behavior and integrate verified replacements before removing existing functionality.
- Build and verify incremental changes. Record any runtime checks that could not be performed.
- Profile representative scenes before making performance claims.
- Keep ROADMAP.md and ARCHITECTURE.md aligned with the implementation. Mark features complete only after implementation and verification.
- Keep OpenGL operations on the context thread and destroy GPU resources before the window.
- Keep the deterministic woodland benchmark and the procedural world playable. Use the benchmark for visual comparisons and record its viewport, build type, and hardware when profiling.
- Preserve continuous camera-relative movement and a fixed gameplay camera. Keep orbit and tilt behind developer controls.
- Keep character proportions, body-part attachments, material settings, and glyph density accessible for manual tuning.
- Generate static surfaces deterministically in world space. Use physical spacing, keep material selection separate from geometry, and include the detail level in GPU cache keys.
- Keep static glyph cutouts on GPU buffers and preserve the dynamic glyph path. Profile warmed-up and cold-cache frames separately.
- Cache world-space normals and intrinsic material data with geometry. Supply time, camera and light changes through GPU uniforms. Keep glyph coverage and depth independent of lighting.
- Keep the legacy Light Map RGB path separate from normal-based lighting. Reuse its visibility channel when integrating the primary torch.
