#include "reversi/player.h"

#include "reversi/movegen.h"

void rv_rng_seed(rv_rng *r, uint64_t seed) { r->s = seed ? seed : UINT64_C(0x9E3779B97F4A7C15); }

uint64_t rv_rng_next(rv_rng *r) {
    uint64_t x = r->s;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    r->s = x;
    return x * UINT64_C(0x2545F4914F6CDD1D);
}

int rv_pick_random(const rv_board *b, rv_rng *r) {
    rv_bb moves = rv_board_moves(b);
    if (moves == 0) {
        return RV_PASS;
    }
    int k = (int)(rv_rng_next(r) % (uint64_t)rv_popcount(moves));
    while (k-- > 0) {
        moves &= moves - 1;
    }
    return rv_ctz(moves);
}

int rv_pick_greedy(const rv_board *b) {
    rv_bb moves = rv_board_moves(b);
    int best = RV_PASS;
    int best_flips = -1;
    while (moves) {
        int sq = rv_ctz(moves);
        moves &= moves - 1;
        int flips = rv_popcount(rv_flips(b->own, b->opp, sq));
        if (flips > best_flips) {
            best_flips = flips;
            best = sq;
        }
    }
    return best;
}
