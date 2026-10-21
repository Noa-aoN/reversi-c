#include "reversi/perft.h"

#include "minitest.h"

static const uint64_t expected[] = {1, 4, 12, 56, 244, 1396, 8200, 55092, 390216, 3005288};

static void test_perft_from_initial_position(void) {
    rv_board b;
    rv_board_init(&b);
    for (int d = 0; d < (int)(sizeof expected / sizeof expected[0]); d++) {
        MT_CHECK_EQ_U64(rv_perft(&b, d), expected[d]);
    }
}

static void test_perft_terminal_position_is_one_leaf(void) {
    rv_board b;
    b.own = RV_BB(0);
    b.opp = RV_BB(63);
    b.side = RV_BLACK;
    MT_CHECK_EQ_U64(rv_perft(&b, 5), 1);
}

static void test_divide_sums_to_perft(void) {
    rv_board b;
    uint64_t per_move[RV_SQUARES];
    rv_board_init(&b);
    for (int d = 1; d <= 6; d++) {
        uint64_t total = rv_perft_divide(&b, d, per_move);
        uint64_t sum = 0;
        int legal = 0;
        for (int sq = 0; sq < RV_SQUARES; sq++) {
            sum += per_move[sq];
            legal += per_move[sq] != 0;
        }
        MT_CHECK_EQ_U64(total, rv_perft(&b, d));
        MT_CHECK_EQ_U64(sum, total);
        MT_CHECK(legal == 4);
    }
    MT_CHECK_EQ_U64(rv_perft_divide(&b, 3, per_move), 56);
    MT_CHECK_EQ_U64(per_move[rv_sq_from_str("f5")], 14);
    MT_CHECK_EQ_U64(per_move[rv_sq_from_str("a1")], 0);
}

int main(void) {
    MT_RUN(test_perft_from_initial_position);
    MT_RUN(test_perft_terminal_position_is_one_leaf);
    MT_RUN(test_divide_sums_to_perft);
    return mt_report("test_perft");
}
