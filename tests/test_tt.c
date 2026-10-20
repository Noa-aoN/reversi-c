#include "reversi/tt.h"

#include "reversi/player.h"

#include "minitest.h"

static rv_tt_entry storage[1024];

static void test_init_requires_power_of_two(void) {
    rv_tt tt;
    MT_CHECK(!rv_tt_init(&tt, NULL, 1024));
    MT_CHECK(!rv_tt_init(&tt, storage, 1000));
    MT_CHECK(rv_tt_init(&tt, storage, 1024));
    MT_CHECK(tt.mask == 1023);
}

static void test_hash_distinguishes_sides(void) {
    rv_board b;
    rv_board_init(&b);
    rv_board swapped = {b.opp, b.own, RV_WHITE};
    MT_CHECK(rv_hash(&b) != rv_hash(&swapped));
    rv_board moved = b;
    rv_board_play(&moved, rv_sq_from_str("f5"));
    MT_CHECK(rv_hash(&b) != rv_hash(&moved));
}

static void test_store_probe_roundtrip(void) {
    rv_tt tt;
    rv_board b;
    rv_tt_init(&tt, storage, 1024);
    rv_board_init(&b);
    MT_CHECK(rv_tt_probe(&tt, &b, rv_hash(&b)) == NULL);
    rv_tt_store(&tt, &b, rv_hash(&b), 4, 123, RV_TT_LOWER, 19);
    const rv_tt_entry *e = rv_tt_probe(&tt, &b, rv_hash(&b));
    MT_CHECK(e != NULL && e->score == 123 && e->depth == 4 && e->flag == RV_TT_LOWER &&
             e->move == 19);
    rv_board other = b;
    rv_board_play(&other, 19);
    MT_CHECK(rv_tt_probe(&tt, &other, rv_hash(&other)) == NULL);
}

static void test_deeper_entry_is_kept(void) {
    rv_tt tt;
    rv_board b;
    rv_tt_init(&tt, storage, 1024);
    rv_board_init(&b);
    rv_tt_store(&tt, &b, rv_hash(&b), 6, 1, RV_TT_EXACT, 19);
    rv_tt_store(&tt, &b, rv_hash(&b), 3, 2, RV_TT_EXACT, 26);
    MT_CHECK(rv_tt_probe(&tt, &b, rv_hash(&b))->score == 1);
    rv_tt_store(&tt, &b, rv_hash(&b), 6, 3, RV_TT_EXACT, 26);
    MT_CHECK(rv_tt_probe(&tt, &b, rv_hash(&b))->score == 3);
}

static void test_clear_forgets_everything(void) {
    rv_tt tt;
    rv_board b;
    rv_tt_init(&tt, storage, 1024);
    rv_board_init(&b);
    rv_tt_store(&tt, &b, rv_hash(&b), 1, 5, RV_TT_EXACT, 19);
    rv_tt_clear(&tt);
    MT_CHECK(rv_tt_probe(&tt, &b, rv_hash(&b)) == NULL);
}

static void test_collisions_never_return_wrong_position(void) {
    rv_tt tt;
    rv_rng r;
    rv_tt_init(&tt, storage, 16);
    rv_rng_seed(&r, 9);
    for (int g = 0; g < 100; g++) {
        rv_board b;
        rv_board_init(&b);
        for (int i = 0; i < 20 && !rv_board_is_over(&b); i++) {
            int sq = rv_pick_random(&b, &r);
            if (sq == RV_PASS) {
                rv_board_pass(&b);
                continue;
            }
            rv_board_play(&b, sq);
            rv_tt_store(&tt, &b, rv_hash(&b), 1, (int)(b.own & 0xFFFF), RV_TT_EXACT, sq);
            const rv_tt_entry *e = rv_tt_probe(&tt, &b, rv_hash(&b));
            MT_CHECK(e == NULL || (e->own == b.own && e->opp == b.opp));
        }
    }
}

int main(void) {
    MT_RUN(test_init_requires_power_of_two);
    MT_RUN(test_hash_distinguishes_sides);
    MT_RUN(test_store_probe_roundtrip);
    MT_RUN(test_deeper_entry_is_kept);
    MT_RUN(test_clear_forgets_everything);
    MT_RUN(test_collisions_never_return_wrong_position);
    return mt_report("test_tt");
}
