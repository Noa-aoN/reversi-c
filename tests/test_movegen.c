#include "reversi/movegen.h"

#include "minitest.h"

#define SQ(s) rv_sq_from_str(s)
#define B(s) RV_BB(SQ(s))

static void test_opening_moves(void) {
    rv_bb black = B("e4") | B("d5");
    rv_bb white = B("d4") | B("e5");
    rv_bb want = B("d3") | B("c4") | B("f5") | B("e6");
    MT_CHECK_EQ_U64(rv_legal_moves(black, white), want);
    rv_bb want_w = B("e3") | B("f4") | B("c5") | B("d6");
    MT_CHECK_EQ_U64(rv_legal_moves(white, black), want_w);
}

static void test_opening_flips(void) {
    rv_bb black = B("e4") | B("d5");
    rv_bb white = B("d4") | B("e5");
    MT_CHECK_EQ_U64(rv_flips(black, white, SQ("f5")), B("e5"));
    MT_CHECK_EQ_U64(rv_flips(black, white, SQ("d3")), B("d4"));
    MT_CHECK_EQ_U64(rv_flips(black, white, SQ("a1")), 0);
    MT_CHECK_EQ_U64(rv_flips(black, white, SQ("f4")), 0);
}

static void test_flips_in_multiple_directions(void) {
    rv_bb own = B("a1") | B("a3") | B("c1") | B("e5");
    rv_bb opp = B("b2") | B("b3") | B("c2") | B("d4");
    rv_bb want = B("b2") | B("b3") | B("c2") | B("d4");
    MT_CHECK_EQ_U64(rv_flips(own, opp, SQ("c3")), want);
}

static void test_flips_stop_at_empty_and_edge(void) {
    rv_bb own = 0;
    rv_bb opp = B("b1") | B("c1") | B("d1") | B("e1") | B("f1") | B("g1") | B("h1");
    MT_CHECK_EQ_U64(rv_flips(own, opp, SQ("a1")), 0);
    own = B("h1");
    opp = B("b1") | B("c1") | B("d1") | B("e1") | B("f1") | B("g1");
    MT_CHECK_EQ_U64(rv_flips(own, opp, SQ("a1")), opp);
    opp = B("b1") | B("c1") | B("e1") | B("f1") | B("g1");
    MT_CHECK_EQ_U64(rv_flips(own, opp, SQ("a1")), 0);
}

static void test_no_wraparound_across_files(void) {
    rv_bb own = B("a2");
    rv_bb opp = B("h1");
    MT_CHECK_EQ_U64(rv_legal_moves(own, opp), 0);
    MT_CHECK_EQ_U64(rv_flips(own, opp, SQ("g1")), 0);
}

int main(void) {
    MT_RUN(test_opening_moves);
    MT_RUN(test_opening_flips);
    MT_RUN(test_flips_in_multiple_directions);
    MT_RUN(test_flips_stop_at_empty_and_edge);
    MT_RUN(test_no_wraparound_across_files);
    return mt_report("test_movegen");
}
