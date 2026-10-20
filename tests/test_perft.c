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

int main(void) {
    MT_RUN(test_perft_from_initial_position);
    MT_RUN(test_perft_terminal_position_is_one_leaf);
    return mt_report("test_perft");
}
