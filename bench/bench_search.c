#include <stdio.h>
#include <stdlib.h>

#include "reversi/player.h"
#include "reversi/search.h"

/* 20 fixed midgame positions (seeded random openings) searched to a fixed depth.
 * Same positions every run, so the numbers are comparable across commits. */

int main(int argc, char **argv) {
    int depth = (argc > 1) ? atoi(argv[1]) : 8;
    rv_rng r;
    uint64_t nodes = 0;
    double seconds = 0;
    rv_rng_seed(&r, 20261020);
    rv_search_clear_tt();
    for (int i = 0; i < 20; i++) {
        rv_board b;
        rv_search_result res;
        rv_board_init(&b);
        for (int p = 0; p < 16; p++) {
            int sq = rv_pick_random(&b, &r);
            if (sq == RV_PASS) {
                rv_board_pass(&b);
            } else {
                rv_board_play(&b, sq);
            }
        }
        rv_search_id(&b, depth, &res);
        nodes += res.nodes;
        seconds += res.seconds;
    }
    printf("search depth %d x 20 positions: %llu nodes in %.3fs (%.2f M nodes/s)\n", depth,
           (unsigned long long)nodes, seconds, (double)nodes / seconds / 1e6);
    return 0;
}
