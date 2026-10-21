#include "reversi/bitboard.h"

#include <string.h>

#include "minitest.h"

#define SQ(s) rv_sq_from_str(s)
#define B(s) RV_BB(SQ(s))

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

static void test_flip_and_mirror(void) {
    MT_CHECK_EQ_U64(rv_flip_vertical(B("a1")), B("a8"));
    MT_CHECK_EQ_U64(rv_flip_vertical(B("h8")), B("h1"));
    MT_CHECK_EQ_U64(rv_flip_vertical(B("d3")), B("d6"));
    MT_CHECK_EQ_U64(rv_mirror_horizontal(B("a1")), B("h1"));
    MT_CHECK_EQ_U64(rv_mirror_horizontal(B("c5")), B("f5"));
    MT_CHECK_EQ_U64(rv_flip_vertical(RV_FILE_A), RV_FILE_A);
    MT_CHECK_EQ_U64(rv_mirror_horizontal(RV_FILE_A), RV_FILE_H);
    for (int sq = 0; sq < RV_SQUARES; sq++) {
        rv_bb bit = RV_BB(sq);
        MT_CHECK_EQ_U64(rv_flip_vertical(rv_flip_vertical(bit)), bit);
        MT_CHECK_EQ_U64(rv_mirror_horizontal(rv_mirror_horizontal(bit)), bit);
        MT_CHECK_EQ_U64(rv_flip_vertical(bit), RV_BB(rv_sq(rv_sq_file(sq), 7 - rv_sq_rank(sq))));
        MT_CHECK_EQ_U64(rv_mirror_horizontal(bit), RV_BB(rv_sq(7 - rv_sq_file(sq), rv_sq_rank(sq))));
    }
}

int main(void) {
    MT_RUN(test_shift_does_not_wrap);
    MT_RUN(test_shift_moves_one_square);
    MT_RUN(test_popcount_ctz);
    MT_RUN(test_square_strings);
    MT_RUN(test_flip_and_mirror);
    return mt_report("test_bitboard");
}
