#ifndef REVERSI_BOARD_H
#define REVERSI_BOARD_H

#include <stddef.h>

#include "reversi/bitboard.h"

typedef enum rv_color { RV_BLACK = 0, RV_WHITE = 1 } rv_color;

typedef struct rv_board {
    rv_bb own;
    rv_bb opp;
    rv_color side;
} rv_board;

#define RV_PASS (-1)
#define RV_BOARD_STR_CAP 200

void rv_board_init(rv_board *b);
rv_bb rv_board_moves(const rv_board *b);
bool rv_board_play(rv_board *b, int sq);
void rv_board_pass(rv_board *b);
bool rv_board_is_over(const rv_board *b);
rv_bb rv_board_bits(const rv_board *b, rv_color c);
int rv_board_count(const rv_board *b, rv_color c);
int rv_board_empties(const rv_board *b);
void rv_board_format(const rv_board *b, char *out, size_t cap);
bool rv_board_parse(rv_board *b, const char *s, rv_color side);

#endif
