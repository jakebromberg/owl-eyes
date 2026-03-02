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

1. **Precomputation** (runs once at startup): Generates 5,000 Lorenz attractor points via Euler integration, encodes each as three Q4.4 fixed-point `int8_t` values packed into a single 24-bit `int`. Also builds sin/cos/perspective lookup tables using float math (one-time cost).

2. **Render loop** (runs every frame): All-integer. Unpacks points, applies Y-axis rotation via trig LUTs, applies perspective projection via perspective LUT, maps to screen coordinates, and draws. Uses temporal dithering (even/odd point alternation) to halve per-frame work.

## Conventions

- **Target platform**: eZ80 -- `int` is 24-bit, no FPU. All hot-path math must be integer-only.
- **Fixed-point formats**: Q4.4 for stored coordinates (8-bit), Q8.8 for LUT values (24-bit `int`).
- **Compiler flags**: `-Wall -Wextra -O3` (set in `makefile`).
- Local clang/LSP diagnostics will show errors for CEdev headers (`tice.h`, `graphx.h`, etc.) -- these are expected and not real build errors.
