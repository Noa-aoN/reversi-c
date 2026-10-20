#include "reversi/player.h"

#include "minitest.h"

#define SQ(s) rv_sq_from_str(s)
#define B(s) RV_BB(SQ(s))

static void test_greedy_prefers_most_flips(void) {
    rv_board b;
    b.own = B("a1");
    b.opp = B("b1") | B("c1") | B("a2");
    b.side = RV_BLACK;
    MT_CHECK(rv_pick_greedy(&b) == SQ("d1"));
}

static void test_greedy_tie_breaks_on_lowest_square(void) {
    rv_board b;
    rv_board_init(&b);
    MT_CHECK(rv_pick_greedy(&b) == SQ("d3"));
}

static void test_greedy_returns_pass_without_moves(void) {
    rv_board b;
    b.own = B("a1");
    b.opp = B("h8");
    b.side = RV_BLACK;
    MT_CHECK(rv_pick_greedy(&b) == RV_PASS);
}

static void test_random_is_legal_and_reproducible(void) {
    rv_board b;
    rv_rng r1, r2;
    rv_board_init(&b);
    rv_rng_seed(&r1, 42);
    rv_rng_seed(&r2, 42);
    for (int i = 0; i < 200; i++) {
        int sq = rv_pick_random(&b, &r1);
        MT_CHECK(sq == rv_pick_random(&b, &r2));
        MT_CHECK(sq != RV_PASS && (rv_board_moves(&b) & RV_BB(sq)) != 0);
    }
    rv_bb seen = 0;
    for (int i = 0; i < 100; i++) {
        seen |= RV_BB(rv_pick_random(&b, &r1));
    }
    MT_CHECK_EQ_U64(seen, rv_board_moves(&b));
}

static void test_random_returns_pass_without_moves(void) {
    rv_board b;
    rv_rng r;
    b.own = B("a1");
    b.opp = B("h8");
    b.side = RV_BLACK;
    rv_rng_seed(&r, 0);
    MT_CHECK(rv_pick_random(&b, &r) == RV_PASS);
}

static void test_random_vs_greedy_games_terminate(void) {
    rv_rng r;
    rv_rng_seed(&r, 7);
    for (int g = 0; g < 50; g++) {
        rv_board b;
        rv_board_init(&b);
        int plies = 0;
        while (!rv_board_is_over(&b) && plies < 200) {
            int sq = (b.side == RV_BLACK) ? rv_pick_random(&b, &r) : rv_pick_greedy(&b);
            MT_CHECK(rv_board_play(&b, sq));
            plies++;
        }
        MT_CHECK(rv_board_is_over(&b));
        MT_CHECK(rv_board_count(&b, RV_BLACK) + rv_board_count(&b, RV_WHITE) <= 64);
    }
}

int main(void) {
    MT_RUN(test_greedy_prefers_most_flips);
    MT_RUN(test_greedy_tie_breaks_on_lowest_square);
    MT_RUN(test_greedy_returns_pass_without_moves);
    MT_RUN(test_random_is_legal_and_reproducible);
    MT_RUN(test_random_returns_pass_without_moves);
    MT_RUN(test_random_vs_greedy_games_terminate);
    return mt_report("test_player");
}
