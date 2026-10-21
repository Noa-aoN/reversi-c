#include "reversi/board.h"

#include <string.h>

#include "minitest.h"

#define SQ(s) rv_sq_from_str(s)
#define B(s) RV_BB(SQ(s))

static void test_initial_position(void) {
    rv_board b;
    rv_board_init(&b);
    MT_CHECK(b.side == RV_BLACK);
    MT_CHECK(rv_board_count(&b, RV_BLACK) == 2);
    MT_CHECK(rv_board_count(&b, RV_WHITE) == 2);
    MT_CHECK(rv_board_empties(&b) == 60);
    MT_CHECK_EQ_U64(rv_board_bits(&b, RV_BLACK), B("e4") | B("d5"));
    MT_CHECK_EQ_U64(rv_board_bits(&b, RV_WHITE), B("d4") | B("e5"));
    MT_CHECK(!rv_board_is_over(&b));
}

static void test_play_flips_and_switches_side(void) {
    rv_board b;
    rv_board_init(&b);
    MT_CHECK(rv_board_play(&b, SQ("f5")));
    MT_CHECK(b.side == RV_WHITE);
    MT_CHECK_EQ_U64(rv_board_bits(&b, RV_BLACK), B("e4") | B("d5") | B("e5") | B("f5"));
    MT_CHECK_EQ_U64(rv_board_bits(&b, RV_WHITE), B("d4"));
    MT_CHECK(rv_board_count(&b, RV_BLACK) == 4);
    MT_CHECK(rv_board_count(&b, RV_WHITE) == 1);
}

static void test_illegal_moves_are_rejected(void) {
    rv_board b, before;
    rv_board_init(&b);
    before = b;
    MT_CHECK(!rv_board_play(&b, SQ("a1")));
    MT_CHECK(!rv_board_play(&b, SQ("d4")));
    MT_CHECK(!rv_board_play(&b, -5));
    MT_CHECK(!rv_board_play(&b, 64));
    MT_CHECK(!rv_board_play(&b, RV_PASS));
    MT_CHECK(b.own == before.own && b.opp == before.opp && b.side == before.side);
}

static void test_pass_only_when_no_moves(void) {
    rv_board b;
    b.own = B("a1");
    b.opp = B("h8");
    b.side = RV_BLACK;
    MT_CHECK(rv_board_moves(&b) == 0);
    MT_CHECK(rv_board_play(&b, RV_PASS));
    MT_CHECK(b.side == RV_WHITE);
    MT_CHECK_EQ_U64(rv_board_bits(&b, RV_WHITE), B("h8"));
    MT_CHECK(rv_board_is_over(&b));
}

static void test_shortest_game(void) {
    static const char *moves[] = {"e6", "f4", "e3", "f6", "g5", "d6", "e7", "f5", "c5"};
    rv_board b;
    char buf[RV_BOARD_STR_CAP];
    rv_board_init(&b);
    for (size_t i = 0; i < sizeof moves / sizeof moves[0]; i++) {
        MT_CHECK(rv_board_play(&b, SQ(moves[i])));
    }
    MT_CHECK(rv_board_is_over(&b));
    MT_CHECK(rv_board_count(&b, RV_BLACK) == 13);
    MT_CHECK(rv_board_count(&b, RV_WHITE) == 0);
    rv_board_format(&b, buf, sizeof buf);
    MT_CHECK(strstr(buf, "game over  X:13 O:0\n") != NULL);
}

static void test_format(void) {
    rv_board b;
    char buf[RV_BOARD_STR_CAP];
    rv_board_init(&b);
    rv_board_format(&b, buf, sizeof buf);
    MT_CHECK(strstr(buf, "  a b c d e f g h\n") == buf);
    MT_CHECK(strstr(buf, "4 - - . O X - - -\n") != NULL);
    MT_CHECK(strstr(buf, "5 - - - X O . - -\n") != NULL);
    MT_CHECK(strstr(buf, "X to move  X:2 O:2\n") != NULL);
}

static void test_parse_reads_format_back(void) {
    rv_board b, parsed;
    char text[RV_BOARD_STR_CAP];
    rv_board_init(&b);
    rv_board_play(&b, SQ("f5"));
    rv_board_play(&b, SQ("d6"));
    rv_board_play(&b, SQ("c3"));
    rv_board_format(&b, text, sizeof text);
    MT_CHECK(rv_board_parse(&parsed, text, b.side));
    MT_CHECK(parsed.own == b.own && parsed.opp == b.opp && parsed.side == b.side);
    MT_CHECK(rv_board_parse(&parsed, text, RV_BLACK));
    MT_CHECK(parsed.own == rv_board_bits(&b, RV_BLACK) && parsed.opp == rv_board_bits(&b, RV_WHITE));
}

static void test_parse_hand_written_and_malformed(void) {
    rv_board b;
    const char *diagram = "X-------"
                          "--------"
                          "--------"
                          "---XO---"
                          "---OX---"
                          "--------"
                          "--------"
                          "-------O";
    MT_CHECK(rv_board_parse(&b, diagram, RV_WHITE));
    MT_CHECK_EQ_U64(rv_board_bits(&b, RV_BLACK), B("a1") | B("d4") | B("e5"));
    MT_CHECK_EQ_U64(rv_board_bits(&b, RV_WHITE), B("e4") | B("d5") | B("h8"));
    MT_CHECK(b.side == RV_WHITE);
    MT_CHECK(!rv_board_parse(&b, "X-------", RV_BLACK));
    MT_CHECK(!rv_board_parse(&b, "", RV_BLACK));
}

int main(void) {
    MT_RUN(test_initial_position);
    MT_RUN(test_play_flips_and_switches_side);
    MT_RUN(test_illegal_moves_are_rejected);
    MT_RUN(test_pass_only_when_no_moves);
    MT_RUN(test_shortest_game);
    MT_RUN(test_format);
    MT_RUN(test_parse_reads_format_back);
    MT_RUN(test_parse_hand_written_and_malformed);
    return mt_report("test_board");
}
