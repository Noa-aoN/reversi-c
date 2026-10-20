#include "reversi/search.h"

#include <stddef.h>

#include "reversi/eval.h"
#include "reversi/movegen.h"

#define BB(file, rank) RV_BB((rank) * 8 + (file))

static const rv_bb CORNERS = BB(0, 0) | BB(7, 0) | BB(0, 7) | BB(7, 7);
static const rv_bb X_SQUARES = BB(1, 1) | BB(6, 1) | BB(1, 6) | BB(6, 6);
static const rv_bb C_SQUARES = BB(1, 0) | BB(0, 1) | BB(6, 0) | BB(7, 1) | BB(0, 6) | BB(1, 7) |
                               BB(7, 6) | BB(6, 7);

typedef struct ctx {
    uint64_t nodes;
} ctx;

static int negamax(ctx *c, const rv_board *b, int depth, int alpha, int beta);

static int search_moves(ctx *c, const rv_board *b, rv_bb moves, int depth, int alpha, int beta,
                        int *best_move) {
    int best = -RV_SCORE_INF;
    while (moves) {
        int sq = rv_ctz(moves);
        moves &= moves - 1;
        rv_board next = *b;
        rv_board_play(&next, sq);
        int score = -negamax(c, &next, depth - 1, -beta, -alpha);
        if (score > best) {
            best = score;
            if (best_move) {
                *best_move = sq;
            }
            if (best > alpha) {
                alpha = best;
                if (alpha >= beta) {
                    break;
                }
            }
        }
    }
    return best;
}

static int ordered_search(ctx *c, const rv_board *b, rv_bb moves, int first, int depth, int alpha,
                          int beta, int *best_move) {
    static const int BUCKETS = 4;
    int best = -RV_SCORE_INF;
    if (first >= 0 && (moves & RV_BB(first))) {
        best = search_moves(c, b, RV_BB(first), depth, alpha, beta, best_move);
        moves &= ~RV_BB(first);
        if (best > alpha) {
            alpha = best;
        }
    }
    for (int i = 0; i < BUCKETS && alpha < beta && moves; i++) {
        rv_bb bucket = (i == 0)   ? moves & CORNERS
                       : (i == 1) ? moves & ~(CORNERS | X_SQUARES | C_SQUARES)
                       : (i == 2) ? moves & C_SQUARES
                                  : moves & X_SQUARES;
        if (!bucket) {
            continue;
        }
        int move = -1;
        int score = search_moves(c, b, bucket, depth, alpha, beta, &move);
        if (score > best) {
            best = score;
            if (best_move) {
                *best_move = move;
            }
            if (best > alpha) {
                alpha = best;
            }
        }
    }
    return best;
}

static int negamax(ctx *c, const rv_board *b, int depth, int alpha, int beta) {
    c->nodes++;
    rv_bb moves = rv_board_moves(b);
    if (moves == 0) {
        rv_board next = *b;
        rv_board_pass(&next);
        if (rv_board_moves(&next) == 0) {
            return rv_eval_final(b);
        }
        return -negamax(c, &next, depth, -beta, -alpha);
    }
    if (depth <= 0) {
        return rv_eval(b);
    }
    return ordered_search(c, b, moves, -1, depth, alpha, beta, NULL);
}

static void search_root(ctx *c, const rv_board *b, int depth, int first, rv_search_result *out) {
    rv_bb moves = rv_board_moves(b);
    out->move = RV_PASS;
    out->depth = depth;
    c->nodes++;
    if (moves == 0) {
        rv_board next = *b;
        rv_board_pass(&next);
        out->score = rv_board_is_over(b)
                         ? rv_eval_final(b)
                         : -negamax(c, &next, depth, -RV_SCORE_INF, RV_SCORE_INF);
        return;
    }
    out->score =
        ordered_search(c, b, moves, first, depth, -RV_SCORE_INF, RV_SCORE_INF, &out->move);
}

void rv_search(const rv_board *b, int depth, rv_search_result *out) {
    ctx c = {0};
    search_root(&c, b, depth, -1, out);
    out->nodes = c.nodes;
}

void rv_search_id(const rv_board *b, int max_depth, rv_search_result *out) {
    ctx c = {0};
    int first = -1;
    int empties = rv_board_empties(b);
    if (max_depth > empties) {
        max_depth = empties;
    }
    if (max_depth < 1) {
        max_depth = 1;
    }
    for (int d = 1; d <= max_depth; d++) {
        search_root(&c, b, d, first, out);
        first = out->move;
    }
    out->nodes = c.nodes;
}
