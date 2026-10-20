#include "reversi/movegen.h"

rv_bb rv_legal_moves(rv_bb own, rv_bb opp) {
    rv_bb empty = ~(own | opp);
    rv_bb moves = 0;
    for (int d = 0; d < RV_DIRS; d++) {
        rv_bb f = rv_shift(own, d) & opp;
        for (int i = 0; i < 5; i++) {
            f |= rv_shift(f, d) & opp;
        }
        moves |= rv_shift(f, d) & empty;
    }
    return moves;
}

rv_bb rv_flips(rv_bb own, rv_bb opp, int sq) {
    rv_bb bit = RV_BB(sq);
    rv_bb flips = 0;
    for (int d = 0; d < RV_DIRS; d++) {
        rv_bb f = rv_shift(bit, d) & opp;
        for (int i = 0; i < 5; i++) {
            f |= rv_shift(f, d) & opp;
        }
        if (rv_shift(f, d) & own) {
            flips |= f;
        }
    }
    return flips;
}
