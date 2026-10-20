#include "reversi/search.h"

#include "reversi/eval.h"
#include "reversi/player.h"

#include "minitest.h"

#define SQ(s) rv_sq_from_str(s)
#define B(s) RV_BB(SQ(s))

static int reference_negamax(const rv_board *b, int depth) {
    rv_bb moves = rv_board_moves(b);
    if (moves == 0) {
        rv_board next = *b;
        rv_board_pass(&next);
        if (rv_board_moves(&next) == 0) {
            return rv_eval_final(b);
        }
        return -reference_negamax(&next, depth);
    }
    if (depth <= 0) {
        return rv_eval(b);
    }
    int best = -RV_SCORE_INF;
    while (moves) {
        int sq = rv_ctz(moves);
        moves &= moves - 1;
        rv_board next = *b;
        rv_board_play(&next, sq);
        int score = -reference_negamax(&next, depth - 1);
        if (score > best) {
            best = score;
        }
    }
    return best;
}

static void random_position(rv_board *b, rv_rng *r, int plies) {
    rv_board_init(b);
    for (int i = 0; i < plies && !rv_board_is_over(b); i++) {
        int sq = rv_pick_random(b, r);
        if (sq == RV_PASS) {
            rv_board_pass(b);
        } else {
            rv_board_play(b, sq);
        }
    }
}

static void test_alpha_beta_matches_plain_negamax(void) {
    rv_rng r;
    rv_rng_seed(&r, 11);
    for (int g = 0; g < 40; g++) {
        rv_board b;
        rv_search_result res;
        random_position(&b, &r, 8 + (int)(rv_rng_next(&r) % 40));
        if (rv_board_moves(&b) == 0) {
            continue;
        }
        rv_search(&b, 3, &res);
        MT_CHECK(res.score == reference_negamax(&b, 3));
        MT_CHECK((rv_board_moves(&b) & RV_BB(res.move)) != 0);
    }
}

static void test_endgame_is_solved_exactly(void) {
    rv_rng r;
    rv_rng_seed(&r, 5);
    int solved = 0;
    for (int g = 0; g < 200 && solved < 15; g++) {
        rv_board b;
        rv_search_result res;
        random_position(&b, &r, 52);
        int empties = rv_board_empties(&b);
        if (rv_board_is_over(&b) || empties > 8 || rv_board_moves(&b) == 0) {
            continue;
        }
        rv_search(&b, empties, &res);
        MT_CHECK(res.score == reference_negamax(&b, empties));
        MT_CHECK(res.score % RV_SCORE_DISC == 0);
        solved++;
    }
    MT_CHECK(solved == 15);
}

static void test_iterative_deepening_reports_depth_and_legal_move(void) {
    rv_board b;
    rv_search_result res;
    rv_board_init(&b);
    rv_search_id(&b, 5, &res);
    MT_CHECK(res.depth == 5);
    MT_CHECK((rv_board_moves(&b) & RV_BB(res.move)) != 0);
    MT_CHECK(res.nodes > 0);
    rv_board late = {B("a1") | B("b1") | B("c1") | B("d1"), B("e1"), RV_BLACK};
    rv_search_id(&late, 60, &res);
    MT_CHECK(res.move == SQ("f1"));
    MT_CHECK(res.score % RV_SCORE_DISC == 0);
}

static void test_takes_winning_move_in_endgame(void) {
    rv_board b = {B("a1") | B("b1") | B("c1") | B("d1"), B("e1"), RV_BLACK};
    rv_search_result res;
    rv_search(&b, 1, &res);
    MT_CHECK(res.move == SQ("f1"));
    MT_CHECK(res.score == 6 * RV_SCORE_DISC);
}

static void test_search_beats_greedy(void) {
    rv_rng r;
    rv_rng_seed(&r, 2026);
    int wins = 0, games = 20;
    for (int g = 0; g < games; g++) {
        rv_board b;
        rv_color ab_color = (g % 2 == 0) ? RV_BLACK : RV_WHITE;
        random_position(&b, &r, 4);
        while (!rv_board_is_over(&b)) {
            int sq;
            if (b.side == ab_color) {
                rv_search_result res;
                rv_search(&b, 3, &res);
                sq = res.move;
            } else {
                sq = rv_pick_greedy(&b);
            }
            MT_CHECK(rv_board_play(&b, sq));
        }
        if (rv_board_count(&b, ab_color) > rv_board_count(&b, ab_color ^ 1)) {
            wins++;
        }
    }
    MT_CHECK(wins * 100 / games >= 70);
}

int main(void) {
    MT_RUN(test_alpha_beta_matches_plain_negamax);
    MT_RUN(test_endgame_is_solved_exactly);
    MT_RUN(test_iterative_deepening_reports_depth_and_legal_move);
    MT_RUN(test_takes_winning_move_in_endgame);
    MT_RUN(test_search_beats_greedy);
    return mt_report("test_search");
}
