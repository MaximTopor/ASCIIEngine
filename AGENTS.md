# ASCIIEngine project rules

- Compose all visible world geometry from ASCII glyphs. Font atlases and other technical textures may support rendering.
- Keep physical object dimensions, visual glyph density, logical cell dimensions, and camera zoom independent.
- Preserve working behavior and integrate verified replacements before removing existing functionality.
- Build and verify incremental changes. Record any runtime checks that could not be performed.
- Profile representative scenes before making performance claims.
- Keep ROADMAP.md and ARCHITECTURE.md aligned with the implementation. Mark features complete only after implementation and verification.
- Keep OpenGL operations on the context thread and destroy GPU resources before the window.
