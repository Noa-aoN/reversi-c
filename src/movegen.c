#include "reversi/movegen.h"

/* Kogge-Stone occluded fill: a run of up to 6 opponent stones is covered by
 * shifts of 1, 2 and 4 instead of a 6-step loop. Wrap-around is prevented by
 * masking off the n files that a shift of n squares would smear across. */

#define LEFT_FILES(n) (RV_FILE_A * ((UINT64_C(1) << (n)) - 1))
#define RIGHT_FILES(n) (LEFT_FILES(n) << (8 - (n)))
#define ALL (~(rv_bb)0)

static const int STEP[RV_DIRS] = {-8, 8, 1, -1, -7, -9, 9, 7};

static const rv_bb MASK1[RV_DIRS] = {ALL,           ALL,           ~LEFT_FILES(1),
                                     ~RIGHT_FILES(1), ~LEFT_FILES(1), ~RIGHT_FILES(1),
                                     ~LEFT_FILES(1), ~RIGHT_FILES(1)};
static const rv_bb MASK2[RV_DIRS] = {ALL,           ALL,           ~LEFT_FILES(2),
                                     ~RIGHT_FILES(2), ~LEFT_FILES(2), ~RIGHT_FILES(2),
                                     ~LEFT_FILES(2), ~RIGHT_FILES(2)};
static const rv_bb MASK4[RV_DIRS] = {ALL,           ALL,           ~LEFT_FILES(4),
                                     ~RIGHT_FILES(4), ~LEFT_FILES(4), ~RIGHT_FILES(4),
                                     ~LEFT_FILES(4), ~RIGHT_FILES(4)};

static inline rv_bb step(rv_bb b, int d, int n) {
    int s = STEP[d] * n;
    return (s >= 0) ? b << s : b >> -s;
}

static inline rv_bb fill(rv_bb gen, rv_bb pro, int d) {
    gen |= pro & (step(gen, d, 1) & MASK1[d]);
    pro &= step(pro, d, 1) & MASK1[d];
    gen |= pro & (step(gen, d, 2) & MASK2[d]);
    pro &= step(pro, d, 2) & MASK2[d];
    gen |= pro & (step(gen, d, 4) & MASK4[d]);
    return gen;
}

rv_bb rv_legal_moves(rv_bb own, rv_bb opp) {
    rv_bb empty = ~(own | opp);
    rv_bb moves = 0;
    for (int d = 0; d < RV_DIRS; d++) {
        rv_bb run = fill(own, opp, d) & ~own;
        moves |= step(run, d, 1) & MASK1[d] & empty;
    }
    return moves;
}

rv_bb rv_flips(rv_bb own, rv_bb opp, int sq) {
    rv_bb bit = RV_BB(sq);
    rv_bb flips = 0;
    for (int d = 0; d < RV_DIRS; d++) {
        rv_bb run = fill(bit, opp, d) & ~bit;
        if (step(run, d, 1) & MASK1[d] & own) {
            flips |= run;
        }
    }
    return flips;
}
