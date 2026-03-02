# Owl Eyes

## Project

A Lorenz attractor renderer for the TI-84 CE Plus calculator (eZ80 processor).

## Build

```sh
make clean && make
```

Requires the [CEdev](https://github.com/CE-Programming/toolchain) toolchain (`cedev-config` must be on `$PATH`). Produces `bin/OWLEYES.8xp`.

## Architecture

Single source file: `src/main.c`. Two phases:

1. **Precomputation** (runs once at startup): Generates 2,500 Lorenz attractor points via Euler integration with adaptive time stepping (smaller steps in fast-transit regions, larger in dense lobe centers). Encodes each as three Q4.4 fixed-point `int8_t` values packed into a single 24-bit `int`. Also builds sin/cos/perspective lookup tables and a Y-axis frustum culling threshold using float math (one-time cost).

2. **Render loop** (runs every frame): All-integer. For each stored point, unpacks coordinates, applies early Y-axis culling, computes factored Y-axis rotation products, and renders both the original point and its `(-x, -y, z)` mirror (exploiting Lorenz Z2 symmetry). Uses perspective LUT, depth-based coloring (8-band white-to-blue gradient), temporal dithering (even/odd point alternation), and 4x manual loop unrolling. The `PROCESS_POINT` macro encapsulates the per-point body.

## Conventions

- **Target platform**: eZ80 -- `int` is 24-bit, no FPU. All hot-path math must be integer-only.
- **Fixed-point formats**: Q4.4 for stored coordinates (8-bit), Q8.8 for LUT values (24-bit `int`).
- **STORE_SCALE**: `0.15` -- maps Lorenz values into Q4.4 range. `D_FX` and `DISPLAY_SCALE_Q8` must be adjusted together with this value to keep on-screen size consistent.
- **Compiler flags**: `-Wall -Wextra -O3` (set in `makefile`).
- **CI**: `.github/workflows/build.yml` -- builds with CEdev v14.2 on Ubuntu, uploads `.8xp` artifact. The makefile sets `SHELL=/bin/zsh`; CI overrides with `SHELL=/bin/bash`.
- Local clang/LSP diagnostics will show errors for CEdev headers (`tice.h`, `graphx.h`, etc.) -- these are expected and not real build errors.
