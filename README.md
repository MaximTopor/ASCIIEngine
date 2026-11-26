# ASCIIEngine

ASCIIEngine renders a playable 2.5D world from ASCII glyphs. The woodland benchmark opens by default. Use `--procedural` for the larger generated world.

```powershell
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure
Set-Location cmake-build-debug
.\ASCIIEngine.exe --procedural
```

Use the configured CLion MinGW environment for building and testing. The GPU shader test needs an OpenGL 4.5 context. CTest marks it skipped if none is available.

## Controls

- **WASD** moves relative to the camera. The mouse wheel zooms and **R** resets zoom.
- **L** toggles the torch and **P** pauses daylight progression. **1–4** select noon, sunset, midnight and sunrise.
- **F3** toggles camera debugging. Use the arrow keys to orbit or tilt when enabled.
- **F4** cycles lighting views and **F5** restores final lighting.

For a material comparison, launch without arguments and look for the two matched samples near the starting position. The steel sample has a brighter, narrower specular highlight than the matte stone sample. Cycle to **Specular**, **Roughness**, or **Reflectivity** to isolate the difference.

[Architecture](ARCHITECTURE.md) describes the lighting pipeline and tuning parameters. [Roadmap](ROADMAP.md) records the tests and measured performance.
