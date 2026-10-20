#ifndef REVERSI_TT_H
#define REVERSI_TT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "reversi/board.h"

enum rv_tt_flag { RV_TT_EXACT = 0, RV_TT_LOWER = 1, RV_TT_UPPER = 2 };

typedef struct rv_tt_entry {
    rv_bb own;
    rv_bb opp;
    int32_t score;
    int8_t depth;
    int8_t flag;
    int8_t move;
    int8_t used;
} rv_tt_entry;

typedef struct rv_tt {
    rv_tt_entry *entries;
    size_t mask;
} rv_tt;

uint64_t rv_hash(const rv_board *b);

bool rv_tt_init(rv_tt *tt, rv_tt_entry *storage, size_t count);
void rv_tt_clear(rv_tt *tt);
const rv_tt_entry *rv_tt_probe(const rv_tt *tt, const rv_board *b, uint64_t hash);
void rv_tt_store(rv_tt *tt, const rv_board *b, uint64_t hash, int depth, int score, int flag,
                 int move);

#endif
