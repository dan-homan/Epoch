# TDLeaf(λ) NNUE Learning — Implementation Reference

## Overview

TDLeaf(λ) is a temporal-difference reinforcement learning algorithm adapted for minimax
search (Baxter, Tridgell, and Weaver, 2000).  It uses the game result and the sequence
of NNUE evaluations at PV leaf positions to form TD errors, then backpropagates those
errors through the NNUE network to update weights.

Build with:

```sh
perl comp.pl 2026_03_09a NNUE=1 TDLEAF=1
```

All learning code is gated by `#if TDLEAF`; when `TDLEAF=0` (default) no overhead is added.

---

## Algorithm

For a game of T half-moves:

- `d_t` = sigmoid of the **NNUE static evaluation at the PV leaf position** at ply t,
  from White's perspective:
  `d_t = 1 / (1 + exp(-score_white_t / K))`, K ≈ 400 cp.
- `z` = game result from White's perspective: 1.0 = White wins, 0.5 = draw, 0.0 = Black wins.

**Temporal difference errors (backward view):**

```
e_{T-1} = z - d_{T-1}
e_t     = (d_{t+1} - d_t) + λ * e_{t+1}     for t = T-2 … 0
```

**Weight update (gradient ascent on prediction accuracy):**

```
Δw = α * Σ_t  e_t * ∇_w d_t
```

where `∇_w d_t = d_t * (1 - d_t) / K * ∇_w score_t`.

Defaults: `λ = 0.7`, `K = 400`, `TDLEAF_ALPHA = 20` (FC), `TDLEAF_ALPHA_FT = 0.008` (FT/PSQT).

**Key design choice:** `d_t` is computed from `nnue_evaluate()` (direct static eval of the
PV leaf), not from the search score propagated from the root.  This ensures the sigmoid
value and the forward-pass gradient are computed from the same NNUE evaluation of the same
position, making the gradient self-consistent.

---

## Scope: All NNUE Weights Are Trained

| Layer | Parameters | Notes |
|-------|-----------|-------|
| FC0 weights/biases | 1,024×16 int8 + 16 int32, ×8 stacks | Quantized int8, float shadow |
| FC1 weights/biases | 32×32 int8 + 32 int32, ×8 stacks | Same |
| FC2 weights/bias   | 32 int8 + 1 int32, ×8 stacks | Same |
| FT weights         | 22,528×1,024 int16 | Sparse update; float shadow on heap (~92 MB) |
| PSQT weights       | 22,528×8 int32 | Sparse update; float shadow (~720 KB) |

FT and PSQT are updated sparsely: only the ~30–60 feature rows active at each leaf
position are touched.  `ft_dirty[FT_INPUTS]` tracks which rows received gradient during
the game; only dirty rows are scanned in `nnue_apply_gradients`.

---

## File Structure

| File | Contents |
|------|----------|
| `src/tdleaf.h` | Hyperparameters, `TDRecord`, `TDGameRecord`, function declarations |
| `src/tdleaf.cpp` | PV walking, TD error computation, gradient backprop hooks |
| `src/nnue.cpp` | FP32 shadow arrays, forward pass, gradient accumulation, weight save/load |
| `src/nnue.h` | `NNUEActivations` struct, `NNUE_FT_LR_SCALE`, TDLeaf function declarations |

---

## Data Structures

### `TDRecord` (per-ply snapshot, stored in `TDGameRecord`)

```cpp
struct TDRecord {
    int16_t acc[2][NNUE_HALF_DIMS];          // accumulator at PV leaf (int16)
    int32_t psqt[2][NNUE_PSQT_BKTS];        // PSQT at PV leaf
    int     score_stm;                        // NNUE static eval at leaf, STM POV (cp)
    int     stack;                            // layer-stack index (piece_count-1)/4
    bool    wtm;                              // White to move at leaf
    int     ft_idx[2][NNUE_MAX_FT_PER_PERSP]; // active FT feature indices
    int8_t  n_ft[2];                          // active feature count per perspective
};
```

Memory: ≈ (2×2048 + 8×4 + 4+4+1 + 2×64×4 + 2) bytes × 400 plies ≈ 2.3 MB.

### `TDGameRecord`

```cpp
struct TDGameRecord {
    TDRecord plies[MAX_GAME_PLY];   // one per half-move (max 400)
    int      n_plies;               // plies recorded so far
};
```

One global instance in `game_rec`; `n_plies` reset to 0 at game start.

---

## PV Leaf Score

`tdleaf_record_ply` walks the PV from the root accumulator using `nnue_record_delta` /
`nnue_apply_delta`, then calls `nnue_evaluate(leaf_acc, leaf_wtm, pc)` to get the leaf
score.  `leaf_wtm = root_wtm XOR (pv_len & 1)` — the side to move at the leaf flips once
per ply walked.

The score is always stored from the leaf's side-to-move perspective (`score_stm`).
`tdleaf_update_after_game` converts to White's perspective as:
`score_white = leaf_wtm ? score_stm : -score_stm`.

To verify correctness at build time:

```sh
perl comp.pl 2026_03_09a NNUE=1 TDLEAF=1 TDLEAF_CHECK_SCORE=1
```

This prints `direct` (NNUE leaf eval) vs `propagated` (root score with per-ply sign flip)
for every ply, flagging differences > 300 cp.

---

## Gradient Flow

```
FC2 output (positional)
  → ∂/∂(positional) via cp_factor and wtm_sign
  → FC2 weights/bias
  → CReLU backward → FC1 pre-activation
  → FC1 weights/bias
  → dual-activation backward (SqrCReLU + CReLU on FC0 outputs 0–14)
  → FC0 pre-activation
  → FC0 weights/bias
  → SqrCReLU backward on accumulator pairs
  → g_acc[2][1024]  (gradient w.r.t. each accumulator unit per perspective)
  → FT weight rows for each active feature index (sparse)
  → PSQT weight rows for each active feature index (sparse)
```

`NNUE_FT_LR_SCALE = TDLEAF_ALPHA_FT / TDLEAF_ALPHA` scales down FT/PSQT gradients
relative to FC.  Update both constants together if you change learning rates.

---

## Weight Persistence — `.tdleaf.bin` (version 3)

Saved at `{exec_path}nn-ad9b42354671.tdleaf.bin`.  Format:

```
[v2 header: version(4) + 8 FC stacks + per-layer weight/bias/count arrays]
[n_ft_rows(4 bytes)]
[per dirty row: fi(4) + ft_w[1024]×128 as int32[1024] + ft_cnt[1024] as uint32[1024]
                      + psqt_w[8]×128 as int32[8]    + psqt_cnt[8]  as uint32[8]]
```

Values are stored at 128× resolution (divide by 128 on load) to preserve sub-integer
drift across sessions.  Update counts enable weighted averaging of concurrent training runs.

Version 2 files (FC only) are still accepted on load; a notice is printed and the file
will be upgraded to version 3 on the next save.

---

## Initialization

When no `.tdleaf.bin` is found, the pretrained Stockfish 15.1 FC weights (from the
`.nnue` file) are used automatically.

To start training from scratch with zeroed FC/FT weights and PSQT = 100 cp/piece:

```sh
./EXchess_v... --init-tdleaf-zero
```

This calls `nnue_init_zero_weights()` and saves the resulting `.tdleaf.bin` immediately.

---

## Hooks in Existing Code

| Location | Change |
|----------|--------|
| `src/define.h` | `#ifndef TDLEAF / #define TDLEAF 0 / #endif` |
| `src/chess.h` — `game_rec` | `TDGameRecord td_game` inside `#if TDLEAF` |
| `src/nnue.cpp` — `nnue_load()` | Calls `nnue_init_fp32_weights()` inside `#if TDLEAF` |
| `src/main.cpp` — after `ts.search()` | `tdleaf_record_ply()` with root acc + PV |
| `src/main.cpp` — `game.over = 1` sites | `tdleaf_update_after_game()` |
| `src/main.cpp` — `new_game` / `setboard` | `td_game.n_plies = 0` |
| `src/EXchess.cc` | `#if TDLEAF #include "tdleaf.cpp" #endif` |
| `src/comp.pl` | `TDLEAF=1` flag → `-D TDLEAF=1` |

---

## Diagnostic Flags

| Flag | Effect |
|------|--------|
| `TDLEAF=1` | Enable all learning code |
| `TDLEAF_READONLY=1` | Load weights but skip gradient updates (inference only) |
| `TDLEAF_CHECK_SCORE=1` | Print direct vs propagated leaf score on every ply |

---

## Self-Play Driver

`run/tdleaf_selfplay.py` manages engine vs engine games, captures results, and runs the
trained binary continuously.  `run/compare_fc_weights.py` compares a `.tdleaf.bin` file
against the baseline `.nnue` and shows FC, FT, and PSQT weight statistics.
