#ifndef REVERSI_BITBOARD_H
#define REVERSI_BITBOARD_H

#include <stdbool.h>
#include <stdint.h>

typedef uint64_t rv_bb;

#define RV_SQUARES 64
#define RV_BB(sq) ((rv_bb)1 << (sq))
#define RV_FILE_A UINT64_C(0x0101010101010101)
#define RV_FILE_H UINT64_C(0x8080808080808080)

enum rv_dir { RV_N, RV_S, RV_E, RV_W, RV_NE, RV_NW, RV_SE, RV_SW, RV_DIRS };

static inline rv_bb rv_shift(rv_bb b, int dir) {
    switch (dir) {
    case RV_N:
        return b >> 8;
    case RV_S:
        return b << 8;
    case RV_E:
        return (b << 1) & ~RV_FILE_A;
    case RV_W:
        return (b >> 1) & ~RV_FILE_H;
    case RV_NE:
        return (b >> 7) & ~RV_FILE_A;
    case RV_NW:
        return (b >> 9) & ~RV_FILE_H;
    case RV_SE:
        return (b << 9) & ~RV_FILE_A;
    case RV_SW:
        return (b << 7) & ~RV_FILE_H;
    default:
        return 0;
    }
}

static inline int rv_popcount(rv_bb b) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcountll(b);
#else
    int n = 0;
    while (b) {
        b &= b - 1;
        n++;
    }
    return n;
#endif
}

static inline int rv_ctz(rv_bb b) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_ctzll(b);
#else
    int n = 0;
    while ((b & 1) == 0) {
        b >>= 1;
        n++;
    }
    return n;
#endif
}

static inline int rv_sq(int file, int rank) { return rank * 8 + file; }
static inline int rv_sq_file(int sq) { return sq & 7; }
static inline int rv_sq_rank(int sq) { return sq >> 3; }

int rv_sq_from_str(const char *s);
void rv_sq_to_str(int sq, char out[3]);

#endif
