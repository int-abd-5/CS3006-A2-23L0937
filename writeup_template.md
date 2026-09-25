# CS3006 Assignment 2 — Performance Analysis on a Multi-Core CPU

**Name:** Muhammad Abdullah Shahid  
**Roll number:** 23L0937  
**Course:** CS3006 Parallel and Distributed Computing, FAST-NUCES Lahore, Fall 2026

> This is a measurement template, not the final submission. Replace every bracketed measurement field only with values collected on the declared Linux/WSL2 machine. Do not use Windows timings or estimated values.

## 1. Machine Declaration

| Field | Measured value |
|---|---|
| CPU model | [run `lscpu`] |
| Physical cores | [run `lscpu`] |
| Hardware threads / logical CPUs | [run `lscpu`] |
| SMT / Hyper-Threading present? | [yes/no from `lscpu`] |
| Base / max clock | [run `lscpu`] |
| Widest SIMD available | [AVX2 W=8 or declared ARM/NEON width] |
| RAM size and type | [measure and identify] |
| Machine type | [own laptop/lab PC/cloud provider and instance] |
| OS, kernel, GCC | [run `uname -a`, `gcc --version`] |
| ISPC version | [run `ispc --version`] |
| Power state | [plugged in; governor if available] |

I followed the addendum measurement protocol: closed unrelated workloads, used AC power, used the performance governor when available, ran each configuration at least five times, reported the minimum, recorded a min/max spread, and allowed long runs to cool.

## 2. Build Notes

- Added `#include <cstring>` to `prog1_mandelbrot_threads/main.cpp`.
- Added `#include <cstdlib>` to `prog1_mandelbrot_threads/mandelbrotThread.cpp`.
- ISPC version: [measured value].
- Other build notes: [only observed changes].

## 3. Program 1 — Threaded Mandelbrot

Record five-run minima and spreads for views 1 and 2 at 1, 2, 3, 4, 5, 6, 7, 8, and 16 threads.

| View | Threads | Serial minimum (ms) | Thread minimum (ms) | Speedup | Spread (min–max ms) |
|---|---:|---:|---:|---:|---|
| 1 | [ ] | [ ] | [ ] | [ ] | [ ] |
| 1 | [ ] | [ ] | [ ] | [ ] | [ ] |
| 2 | [ ] | [ ] | [ ] | [ ] | [ ] |

Explain the initial load imbalance, per-thread timing evidence, the balanced decomposition, the final speedup at `T` threads versus ideal `T`, and the 16-thread result.

## 4. Program 2 — SIMD Intrinsics

Record vector utilization from `./myexp -s 10000` for `VECTOR_WIDTH` 2, 4, 8, and 16. Explain the trend and confirm correctness for a non-divisible input such as `N=17`.

| Vector width | Vector utilization | Correctness |
|---:|---:|---|
| 2 | [ ] | [Passed] |
| 4 | [ ] | [Passed] |
| 8 | [ ] | [Passed] |
| 16 | [ ] | [Passed] |

## 5. Program 3 — ISPC Mandelbrot

The declared SIMD ceiling is `W = [ ]`; the hardware-relative task ceiling is `C × W = [ ]`.

Report serial, SIMD, and task minima and spreads for both views. Express SIMD speedup against `W` and task speedup against `C × W`; explain divergence and task-count choice.

## 6. Program 4 — Iterative `sqrt`

Record default, best-case, and worst-case input timings for serial, ISPC, and task ISPC. Explain how iteration-count uniformity affects SIMD efficiency and how tasking changes multicore speedup.

## 7. Program 5 — SAXPY

Report five-run minima/spreads, ISPC task speedup, observed GB/s, and the theoretical RAM bandwidth for the declared RAM type, speed, and channel count. Explain whether the result is compute- or bandwidth-limited.

## 8. Program 6 — K-Means

- Full dataset checksum: [paste `md5sum data.dat`; expected `3a25f24193f4fdca82ee4cb2737fd5bb`].
- Baseline runtime: [five-run minimum and spread].
- Optimized runtime: [five-run minimum and spread].
- Speedup: [measured baseline / optimized].
- Profiled hotspot fraction: `f = [ ]`.
- Hardware threads: `T = [ ]`.
- Amdahl arithmetic: `Smax = 1 / ((1-f) + f/T) = [show arithmetic]`.
- Required threshold: `0.80 × Smax = [ ]`.
- Achieved fraction of ceiling: [ ]

Narrative: “I measured … which suggested … I tried … resulting in … .” Include the rejected approach, the final `computeAssignments` design, correctness evidence, and references to `plots/start.png` and `plots/end.png`.

## 9. Extra Credit

[Clearly label only work actually attempted and measured.]

## 10. AI Use Disclosure

I used an AI assistant to help understand the handout, organize the implementation, and review code structure. I ran the submitted code and collected the submitted measurements myself on the declared machine; the assistant did not fabricate timings, hardware facts, plots, or conclusions.

## 11. Reproducibility and Final Validation Checklist

Run the following from the repository root on the declared Linux/WSL2 measurement machine. Use the ISPC 1.31.0 executable selected by the addendum and record only measurements from that machine.

```bash
ISPC=/path/to/ispc-v1.31.0-linux/bin/ispc
make -C prog1_mandelbrot_threads clean all
make -C prog2_vecintrin clean all
make -C prog3_mandelbrot_ispc clean all ISPC="$ISPC"
make -C prog4_sqrt clean all ISPC="$ISPC"
make -C prog5_saxpy clean all ISPC="$ISPC"
make -C prog6_kmeans clean all
```

Before packaging, verify the required correctness checks, collect five runs per configuration, save the machine declaration, compute the Program 6 checksum, generate the required plots, and replace every remaining bracketed field in this template. WSL2 smoke-test output is useful for correctness but must not be reported as the final performance measurement unless it is the declared measurement environment.
