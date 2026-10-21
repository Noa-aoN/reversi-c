#include "reversi/perft.h"

#include "reversi/movegen.h"

uint64_t rv_perft(const rv_board *b, int depth) {
    if (depth == 0) {
        return 1;
    }
    /* Reference counts (Aart Bik) treat a forced pass as one ply and a finished
     * game as one leaf; depth 10 is the first that distinguishes the variants. */
    rv_bb moves = rv_legal_moves(b->own, b->opp);
    if (moves == 0) {
        if (rv_legal_moves(b->opp, b->own) == 0) {
            return 1;
        }
        rv_board next = *b;
        rv_board_pass(&next);
        return rv_perft(&next, depth - 1);
    }
    uint64_t nodes = 0;
    while (moves) {
        int sq = rv_ctz(moves);
        moves &= moves - 1;
        rv_board next = *b;
        rv_board_play(&next, sq);
        nodes += rv_perft(&next, depth - 1);
    }
    return nodes;
}

uint64_t rv_perft_divide(const rv_board *b, int depth, uint64_t out[RV_SQUARES]) {
    for (int sq = 0; sq < RV_SQUARES; sq++) {
        out[sq] = 0;
    }
    if (depth == 0) {
        return 1;
    }
    rv_bb moves = rv_legal_moves(b->own, b->opp);
    uint64_t total = 0;
    while (moves) {
        int sq = rv_ctz(moves);
        moves &= moves - 1;
        rv_board next = *b;
        rv_board_play(&next, sq);
        out[sq] = rv_perft(&next, depth - 1);
        total += out[sq];
    }
    return total;
}
