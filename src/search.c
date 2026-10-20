#include "reversi/search.h"

#include <stddef.h>
#include <time.h>

#include "reversi/eval.h"
#include "reversi/movegen.h"
#include "reversi/tt.h"

#define BB(file, rank) RV_BB((rank) * 8 + (file))

#ifndef RV_TT_BITS
#define RV_TT_BITS 18
#endif
#define TIME_CHECK_INTERVAL 4096

static const rv_bb CORNERS = BB(0, 0) | BB(7, 0) | BB(0, 7) | BB(7, 7);
static const rv_bb X_SQUARES = BB(1, 1) | BB(6, 1) | BB(1, 6) | BB(6, 6);
static const rv_bb C_SQUARES = BB(1, 0) | BB(0, 1) | BB(6, 0) | BB(7, 1) | BB(0, 6) | BB(1, 7) |
                               BB(7, 6) | BB(6, 7);

static rv_tt_entry tt_storage[(size_t)1 << RV_TT_BITS];
static rv_tt tt;
static bool tt_ready;

typedef struct ctx {
    uint64_t nodes;
    double deadline;
    bool timed;
    bool aborted;
} ctx;

static double now_seconds(void) {
    struct timespec ts;
    if (timespec_get(&ts, TIME_UTC) != TIME_UTC) {
        return (double)clock() / CLOCKS_PER_SEC;
    }
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static void ensure_tt(void) {
    if (!tt_ready) {
        rv_tt_init(&tt, tt_storage, sizeof tt_storage / sizeof tt_storage[0]);
        tt_ready = true;
    }
}

void rv_search_clear_tt(void) {
    ensure_tt();
    rv_tt_clear(&tt);
}

static int negamax(ctx *c, const rv_board *b, int depth, int alpha, int beta);

static int search_moves(ctx *c, const rv_board *b, rv_bb moves, int depth, int alpha, int beta,
                        int *best_move) {
    int best = -RV_SCORE_INF;
    while (moves && !c->aborted) {
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
    for (int i = 0; i < BUCKETS && alpha < beta && moves && !c->aborted; i++) {
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
    if (c->timed && (c->nodes % TIME_CHECK_INTERVAL) == 0 && now_seconds() >= c->deadline) {
        c->aborted = true;
    }
    if (c->aborted) {
        return 0;
    }
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

    uint64_t hash = rv_hash(b);
    const rv_tt_entry *e = rv_tt_probe(&tt, b, hash);
    int first = -1;
    if (e) {
        first = e->move;
        if (e->depth == depth) {
            if (e->flag == RV_TT_EXACT) {
                return e->score;
            }
            if (e->flag == RV_TT_LOWER && e->score >= beta) {
                return e->score;
            }
            if (e->flag == RV_TT_UPPER && e->score <= alpha) {
                return e->score;
            }
        }
    }

    int alpha0 = alpha;
    int best_move = -1;
    int best = ordered_search(c, b, moves, first, depth, alpha, beta, &best_move);
    if (!c->aborted) {
        int flag = (best <= alpha0) ? RV_TT_UPPER : (best >= beta) ? RV_TT_LOWER : RV_TT_EXACT;
        rv_tt_store(&tt, b, hash, depth, best, flag, best_move);
    }
    return best;
}

static void search_root(ctx *c, const rv_board *b, int depth, int first, rv_search_result *out) {
    rv_bb moves = rv_board_moves(b);
    c->nodes++;
    out->move = RV_PASS;
    out->depth = depth;
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

static void finish(ctx *c, double start, int empties, rv_search_result *out) {
    out->nodes = c->nodes;
    out->seconds = now_seconds() - start;
    out->exact = out->depth >= empties;
}

void rv_search(const rv_board *b, int depth, rv_search_result *out) {
    ctx c = {0};
    double start = now_seconds();
    ensure_tt();
    search_root(&c, b, depth, -1, out);
    finish(&c, start, rv_board_empties(b), out);
}

static void iterate(ctx *c, const rv_board *b, int max_depth, rv_search_result *out) {
    rv_search_result last;
    int first = -1;
    int empties = rv_board_empties(b);
    if (max_depth > empties) {
        max_depth = empties;
    }
    if (max_depth < 1) {
        max_depth = 1;
    }
    for (int d = 1; d <= max_depth; d++) {
        search_root(c, b, d, first, &last);
        if (c->aborted) {
            break;
        }
        *out = last;
        first = out->move;
    }
}

void rv_search_id(const rv_board *b, int max_depth, rv_search_result *out) {
    ctx c = {0};
    double start = now_seconds();
    ensure_tt();
    iterate(&c, b, max_depth, out);
    finish(&c, start, rv_board_empties(b), out);
}

void rv_search_timed(const rv_board *b, int max_depth, double seconds, rv_search_result *out) {
    ctx c = {0};
    double start = now_seconds();
    c.timed = true;
    c.deadline = start + seconds;
    ensure_tt();
    iterate(&c, b, max_depth, out);
    finish(&c, start, rv_board_empties(b), out);
}
