#include "reversi/eval.h"

#include "reversi/movegen.h"

#define BB(file, rank) RV_BB((rank) * 8 + (file))

static const rv_bb CORNERS = BB(0, 0) | BB(7, 0) | BB(0, 7) | BB(7, 7);
static const rv_bb X_SQUARES = BB(1, 1) | BB(6, 1) | BB(1, 6) | BB(6, 6);
static const rv_bb C_SQUARES = BB(1, 0) | BB(0, 1) | BB(6, 0) | BB(7, 1) | BB(0, 6) | BB(1, 7) |
                               BB(7, 6) | BB(6, 7);
static const rv_bb A_SQUARES = BB(2, 0) | BB(5, 0) | BB(0, 2) | BB(7, 2) | BB(0, 5) | BB(7, 5) |
                               BB(2, 7) | BB(5, 7);
static const rv_bb B_SQUARES = BB(3, 0) | BB(4, 0) | BB(0, 3) | BB(7, 3) | BB(0, 4) | BB(7, 4) |
                               BB(3, 7) | BB(4, 7);
static const rv_bb INNER_RING = BB(2, 1) | BB(3, 1) | BB(4, 1) | BB(5, 1) | BB(2, 6) | BB(3, 6) |
                                BB(4, 6) | BB(5, 6) | BB(1, 2) | BB(1, 3) | BB(1, 4) | BB(1, 5) |
                                BB(6, 2) | BB(6, 3) | BB(6, 4) | BB(6, 5);

static const struct {
    rv_bb corner;
    rv_bb x;
    rv_bb c;
} CORNER_ZONE[4] = {
    {BB(0, 0), BB(1, 1), BB(1, 0) | BB(0, 1)},
    {BB(7, 0), BB(6, 1), BB(6, 0) | BB(7, 1)},
    {BB(0, 7), BB(1, 6), BB(0, 6) | BB(1, 7)},
    {BB(7, 7), BB(6, 6), BB(7, 6) | BB(6, 7)},
};

enum {
    W_CORNER = 100,
    W_X = -50,
    W_C = -20,
    W_A = 10,
    W_B = 5,
    W_INNER = -2,
    W_CENTER = -1,
    W_MOBILITY = 8,
};

static int diff(rv_bb own, rv_bb opp, rv_bb mask) {
    return rv_popcount(own & mask) - rv_popcount(opp & mask);
}

int rv_eval_final(const rv_board *b) {
    return (rv_popcount(b->own) - rv_popcount(b->opp)) * RV_SCORE_DISC;
}

int rv_eval(const rv_board *b) {
    rv_bb own = b->own, opp = b->opp;
    rv_bb center = ~(CORNERS | X_SQUARES | C_SQUARES | A_SQUARES | B_SQUARES | INNER_RING);
    int score = W_CORNER * diff(own, opp, CORNERS) + W_X * diff(own, opp, X_SQUARES) +
                W_C * diff(own, opp, C_SQUARES) + W_A * diff(own, opp, A_SQUARES) +
                W_B * diff(own, opp, B_SQUARES) + W_INNER * diff(own, opp, INNER_RING) +
                W_CENTER * diff(own, opp, center);
    for (int i = 0; i < 4; i++) {
        if ((own | opp) & CORNER_ZONE[i].corner) {
            score -= W_X * diff(own, opp, CORNER_ZONE[i].x);
            score -= W_C * diff(own, opp, CORNER_ZONE[i].c);
        }
    }
    score += W_MOBILITY *
             (rv_popcount(rv_legal_moves(own, opp)) - rv_popcount(rv_legal_moves(opp, own)));
    return score;
}
