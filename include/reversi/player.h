#ifndef REVERSI_PLAYER_H
#define REVERSI_PLAYER_H

#include "reversi/board.h"

typedef struct rv_rng {
    uint64_t s;
} rv_rng;

void rv_rng_seed(rv_rng *r, uint64_t seed);
uint64_t rv_rng_next(rv_rng *r);

int rv_pick_random(const rv_board *b, rv_rng *r);
int rv_pick_greedy(const rv_board *b);

#endif
