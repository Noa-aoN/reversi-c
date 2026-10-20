#include "reversi/bitboard.h"

#include <string.h>

#include "minitest.h"

static void test_shift_does_not_wrap(void) {
    rv_bb h1 = RV_BB(rv_sq(7, 0));
    rv_bb a8 = RV_BB(rv_sq(0, 7));
    MT_CHECK(rv_shift(h1, RV_E) == 0);
    MT_CHECK(rv_shift(h1, RV_N) == 0);
    MT_CHECK(rv_shift(h1, RV_NE) == 0);
    MT_CHECK(rv_shift(h1, RV_SE) == 0);
    MT_CHECK(rv_shift(a8, RV_W) == 0);
    MT_CHECK(rv_shift(a8, RV_S) == 0);
    MT_CHECK(rv_shift(a8, RV_SW) == 0);
    MT_CHECK(rv_shift(a8, RV_NW) == 0);
}

static void test_shift_moves_one_square(void) {
    rv_bb d4 = RV_BB(rv_sq(3, 3));
    MT_CHECK(rv_shift(d4, RV_N) == RV_BB(rv_sq(3, 2)));
    MT_CHECK(rv_shift(d4, RV_S) == RV_BB(rv_sq(3, 4)));
    MT_CHECK(rv_shift(d4, RV_E) == RV_BB(rv_sq(4, 3)));
    MT_CHECK(rv_shift(d4, RV_W) == RV_BB(rv_sq(2, 3)));
    MT_CHECK(rv_shift(d4, RV_NE) == RV_BB(rv_sq(4, 2)));
    MT_CHECK(rv_shift(d4, RV_NW) == RV_BB(rv_sq(2, 2)));
    MT_CHECK(rv_shift(d4, RV_SE) == RV_BB(rv_sq(4, 4)));
    MT_CHECK(rv_shift(d4, RV_SW) == RV_BB(rv_sq(2, 4)));
}

static void test_popcount_ctz(void) {
    MT_CHECK(rv_popcount(0) == 0);
    MT_CHECK(rv_popcount(~(rv_bb)0) == 64);
    MT_CHECK(rv_popcount(RV_FILE_A) == 8);
    MT_CHECK(rv_ctz(RV_BB(0)) == 0);
    MT_CHECK(rv_ctz(RV_BB(63)) == 63);
    MT_CHECK(rv_ctz(RV_FILE_H) == 7);
}

static void test_square_strings(void) {
    char buf[3];
    MT_CHECK(rv_sq_from_str("a1") == 0);
    MT_CHECK(rv_sq_from_str("h8") == 63);
    MT_CHECK(rv_sq_from_str("F5") == rv_sq(5, 4));
    MT_CHECK(rv_sq_from_str("i1") == -1);
    MT_CHECK(rv_sq_from_str("a9") == -1);
    MT_CHECK(rv_sq_from_str("a") == -1);
    MT_CHECK(rv_sq_from_str("a1x") == -1);
    MT_CHECK(rv_sq_from_str("") == -1);
    MT_CHECK(rv_sq_from_str(NULL) == -1);
    rv_sq_to_str(rv_sq(5, 4), buf);
    MT_CHECK(strcmp(buf, "f5") == 0);
    for (int sq = 0; sq < RV_SQUARES; sq++) {
        rv_sq_to_str(sq, buf);
        MT_CHECK(rv_sq_from_str(buf) == sq);
    }
}

int main(void) {
    MT_RUN(test_shift_does_not_wrap);
    MT_RUN(test_shift_moves_one_square);
    MT_RUN(test_popcount_ctz);
    MT_RUN(test_square_strings);
    return mt_report("test_bitboard");
}
