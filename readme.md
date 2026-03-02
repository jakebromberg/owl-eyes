### Owl Eyes

A rotating 3D Lorenz attractor for the TI-84 CE Plus calculator.

Renders 2,500 precomputed Lorenz attractor points (plus their Z2-symmetric mirrors) with real-time Y-axis rotation, perspective projection, and depth coloring. Optimized for the eZ80 processor (8-bit CPU, ~48 MHz, no FPU):

- **Q4.4 fixed-point packing** -- Three 8-bit coordinates packed into a single native 24-bit `int`, halving memory usage vs. storing floats.
- **All-integer render loop** -- No floating-point math in the hot path. Trig (sin/cos) and perspective are served from precomputed Q8.8 lookup tables; all intermediate values fit in 24-bit native `int`.
- **Z2 symmetry exploitation** -- Stores half the trajectory and renders both each point and its `(-x, -y, z)` mirror, halving memory usage while maintaining full visual coverage.
- **Adaptive precomputation** -- Variable time step during Euler integration distributes points evenly across the attractor's surface (more points on fast-transit arcs, fewer on dense lobe centers).
- **Depth coloring** -- 8-band white-to-blue gradient mapped from post-rotation z-depth. Near points render bright, far points dim.
- **Temporal point dithering** -- Even/odd points alternate across frames, halving per-frame work while LCD persistence makes the output appear continuous.
- **Early frustum culling** -- Y-axis pre-check skips off-screen points before rotation math.
- **4x loop unrolling** -- Render loop manually unrolled to reduce branch overhead on the branchless eZ80 pipeline.
- **Double buffering** -- Draws to a back buffer and swaps, eliminating flicker.

Uses the [CEdev](https://github.com/CE-Programming/toolchain) toolchain.

#### Building

```sh
make clean && make
```

Produces `bin/OWLEYES.8xp`. Transfer to a TI-84 CE Plus or run in [CEmu](https://ce-programming.github.io/CEmu/).

#### Controls

- **CLEAR** -- Exit the program.
