#include "reversi/eval.h"

#include "reversi/player.h"

#include "minitest.h"

#define SQ(s) rv_sq_from_str(s)
#define B(s) RV_BB(SQ(s))

static void test_antisymmetric_on_random_positions(void) {
    rv_rng r;
    rv_rng_seed(&r, 3);
    for (int g = 0; g < 30; g++) {
        rv_board b;
        rv_board_init(&b);
        int plies = (int)(rv_rng_next(&r) % 50);
        for (int i = 0; i < plies && !rv_board_is_over(&b); i++) {
            int sq = rv_pick_random(&b, &r);
            if (sq == RV_PASS) {
                rv_board_pass(&b);
            } else {
                rv_board_play(&b, sq);
            }
        }
        rv_board flipped = {b.opp, b.own, b.side == RV_BLACK ? RV_WHITE : RV_BLACK};
        MT_CHECK(rv_eval(&b) == -rv_eval(&flipped));
        MT_CHECK(rv_eval_final(&b) == -rv_eval_final(&flipped));
    }
}

static void test_corner_is_worth_more_than_x_square(void) {
    rv_board corner = {B("a1") | B("d4"), B("e5"), RV_BLACK};
    rv_board xsq = {B("b2") | B("d4"), B("e5"), RV_BLACK};
    rv_board plain = {B("d4"), B("e5"), RV_BLACK};
    MT_CHECK(rv_eval(&corner) > rv_eval(&plain));
    MT_CHECK(rv_eval(&xsq) < rv_eval(&plain));
}

static void test_x_square_penalty_vanishes_once_corner_is_taken(void) {
    rv_board open = {B("b2") | B("d4"), B("e5"), RV_BLACK};
    rv_board open_no_x = {B("d4"), B("e5"), RV_BLACK};
    rv_board taken = {B("b2") | B("d4"), B("e5") | B("a1"), RV_BLACK};
    rv_board taken_no_x = {B("d4"), B("e5") | B("a1"), RV_BLACK};
    int x_cost_open = rv_eval(&open) - rv_eval(&open_no_x);
    int x_cost_taken = rv_eval(&taken) - rv_eval(&taken_no_x);
    MT_CHECK(x_cost_open < -30);
    MT_CHECK(x_cost_taken > x_cost_open + 30);
}

static void test_final_score_is_disc_difference(void) {
    rv_board b = {B("a1") | B("b1") | B("c1"), B("h8"), RV_BLACK};
    MT_CHECK(rv_eval_final(&b) == 2 * RV_SCORE_DISC);
}

int main(void) {
    MT_RUN(test_antisymmetric_on_random_positions);
    MT_RUN(test_corner_is_worth_more_than_x_square);
    MT_RUN(test_x_square_penalty_vanishes_once_corner_is_taken);
    MT_RUN(test_final_score_is_disc_difference);
    return mt_report("test_eval");
}
