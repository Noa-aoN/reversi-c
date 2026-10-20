#ifndef REVERSI_SEARCH_H
#define REVERSI_SEARCH_H

#include <stdbool.h>
#include <stdint.h>

#include "reversi/board.h"

#define RV_SEARCH_MAX_DEPTH 60

typedef struct rv_search_result {
    int move;
    int score;
    int depth;
    uint64_t nodes;
    double seconds;
    bool exact;
} rv_search_result;

void rv_search(const rv_board *b, int depth, rv_search_result *out);
void rv_search_id(const rv_board *b, int max_depth, rv_search_result *out);
void rv_search_timed(const rv_board *b, int max_depth, double seconds, rv_search_result *out);
void rv_search_clear_tt(void);

#endif
