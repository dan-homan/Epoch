// EXchess NNUE evaluation — HalfKAv2_hm format (Stockfish 15.1 exact)
//
// Architecture (confirmed from file structure — each stack is 17,640 bytes):
//   Features = HalfKAv2_hm — king-square bucket × all-piece-square
//   Feature space : 22,528  (32 king-buckets × 704 piece-sq indices)
//   Accumulator   : 1024 int16 per perspective  +  8 int32 PSQT per perspective
//   Network       : 8 layer-stacks (selected by material count), each:
//     - FC0: 1,024 → 16  (SqrCReLU only: 512 per perspective × 2 sides = 1024)
//     - FC1: 30   → 32  (30 = 15 SqrCReLU + 15 CReLU from FC0 outputs 0-14)
//     - FC2: 32   → 1   (output; FC0 output-15 adds via passthrough)
//   PSQT : accumulated separately; final score = (psqt_diff/2 + positional) / 16
//   Note: accumulator is NOT shifted before SqrCReLU; clamp directly to [0,127]
//
// Compatible net: nn-ad9b42354671.nnue (Stockfish 15.1 exact release net)

#ifndef NNUE_H
#define NNUE_H

#include <cstdint>

// ---------------------------------------------------------------------------
// Architecture constants
// ---------------------------------------------------------------------------
static const int NNUE_HALF_DIMS    = 1024;  // accumulator units per perspective
static const int NNUE_FT_INPUTS    = 22528; // 32 king-buckets × 704 piece-sq
static const int NNUE_LAYER_STACKS = 8;     // separate nets per material bucket
static const int NNUE_PSQT_BKTS   = 8;     // PSQT buckets (== LAYER_STACKS)

// FC0: SqrCReLU-only input (512 per perspective × 2 sides)
static const int NNUE_L0_SIZE     = 16;   // FC0 output neurons (incl. direct-out)
static const int NNUE_L0_DIRECT   = 15;   // FC0 outputs going through activations
static const int NNUE_L0_INPUT    = 1024; // = 2 × 512 SqrCReLU per side (no CReLU)

// FC1: dense (takes dual-activation of FC0 outputs 0..14 → 15 sqr + 15 clip = 30)
static const int NNUE_L1_SIZE     = 32;   // FC1 output neurons
static const int NNUE_L1_PADDED   = 32;   // padded input dim (ceil(30, 16) = 32)

// FC2 (output): input = NNUE_L1_SIZE = 32 neurons
static const int NNUE_L2_PADDED   = 32;   // padded input dim for FC2

// Quantization scales
static const int NNUE_WEIGHT_SHIFT = 6;   // FC weights: raw >> 6 → [0,127] int8
static const int NNUE_SQR_SHIFT    = 7;   // SqrCReLU: (v*v) >> 7 → [0,127]
// Stockfish 15.1 output formula (internal Value units):
//   value = (psqt_diff/2 + positional) / OutputScale   (OutputScale=16)
// To convert Stockfish Value → EXchess centipawns multiply by 100/NormalizeToPawnValue
//   where NormalizeToPawnValue = 361 (from Stockfish uci.h).
// Combined denominator: OutputScale * NormalizeToPawnValue / 100 = 16 * 3.61 ≈ 58.
// Passthrough (FC0 output 15): fwdOut = fc0_raw[15] * 9600 / 8128
//   (600 * OutputScale) / (127 * WeightScaleBits=64) = 9600/8128
static const int NNUE_OUTPUT_SCALE = 32;  // PSQT scale: empirically confirmed for correct piece values
static const int NNUE_POS_SCALE    = 128; // Positional (FC layers) scale: ~4× PSQT to match EXchess cp

// ---------------------------------------------------------------------------
// Accumulator (one per search node, lazily updated)
// ---------------------------------------------------------------------------
struct NNUEAccumulator {
    int16_t acc [2][NNUE_HALF_DIMS];  // [perspective][unit]  WHITE=1, BLACK=0
    int32_t psqt[2][NNUE_PSQT_BKTS]; // [perspective][bucket]
    bool    dirty[2];                 // true → full refresh needed (legacy fallback)

    // Lazy evaluation: instead of copying and updating at every node, each node
    // records only the feature-index deltas from its parent.  The full accumulator
    // is materialised (via nnue_apply_delta) only when score_pos is actually called.
    bool    computed;           // true → acc[][] is fully up-to-date
    int     add[2][4];          // per-perspective feature indices to add (max 4)
    int     sub[2][4];          // per-perspective feature indices to subtract (max 4)
    int8_t  n_add[2];           // count of adds per perspective
    int8_t  n_sub[2];           // count of subs per perspective
    bool    need_refresh[2];    // true → perspective needs full rebuild (king moved)

    NNUEAccumulator() {
        dirty[0] = dirty[1] = true;
        computed = true;
        n_add[0] = n_add[1] = n_sub[0] = n_sub[1] = 0;
        need_refresh[0] = need_refresh[1] = false;
    }
};

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------
extern bool nnue_available;

// Load a HalfKAv2_hm .nnue file. Returns true on success.
bool nnue_load(const char *path);

// Full accumulator refresh from the current position.
void nnue_init_accumulator(NNUEAccumulator &acc, const struct position &pos);

// Incremental update after exec_move (eager copy-make; kept for reference).
void nnue_update_accumulator(NNUEAccumulator &next_acc,
                             const struct position &before,
                             const struct position &after,
                             union move mv);

// Record feature-index deltas for a move without touching ft_weights.
// Fills acc.add/sub/n_add/n_sub/need_refresh and sets acc.computed = false.
// Call after exec_move with before=pre-move pos and after=post-move pos.
void nnue_record_delta(NNUEAccumulator &acc,
                       const struct position &before,
                       const struct position &after,
                       union move mv);

// Materialise a lazy accumulator from the parent's computed accumulator.
// Copies parent_acc for each non-refresh perspective and applies the stored
// deltas; rebuilds from scratch for need_refresh perspectives.
// Sets acc.computed = true on return.
void nnue_apply_delta(NNUEAccumulator &acc,
                      const NNUEAccumulator &parent_acc,
                      const struct position &pos);

// Forward pass. Returns centipawns from side-to-move's perspective.
// piece_count: total pieces on board (for layer-stack selection).
int nnue_evaluate(const NNUEAccumulator &acc, int stm, int piece_count);

#endif // NNUE_H
