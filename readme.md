### Owl Eyes

A rotating 3D Lorenz attractor for the TI-84 CE Plus calculator.

Renders 5,000 precomputed Lorenz attractor points with real-time Y-axis rotation and perspective projection. Optimized for the eZ80 processor (8-bit CPU, ~48 MHz, no FPU):

- **Q4.4 fixed-point packing** -- Three 8-bit coordinates packed into a single native 24-bit `int`, halving memory usage vs. storing floats.
- **All-integer render loop** -- No floating-point math in the hot path. Trig (sin/cos) and perspective are served from precomputed Q8.8 lookup tables; all intermediate values fit in 24-bit native `int`.
- **Temporal point dithering** -- Even/odd points alternate across frames, halving per-frame work while LCD persistence makes the output appear continuous.
- **Double buffering** -- Draws to a back buffer and swaps, eliminating flicker.

Uses the [CEdev](https://github.com/CE-Programming/toolchain) toolchain.

#### Building

```sh
make clean && make
```

Produces `bin/OWLEYES.8xp`. Transfer to a TI-84 CE Plus or run in [CEmu](https://ce-programming.github.io/CEmu/).

#### Controls

- **CLEAR** -- Exit the program.
