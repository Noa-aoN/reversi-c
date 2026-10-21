#ifndef REVERSI_PERFT_H
#define REVERSI_PERFT_H

#include <stdint.h>

#include "reversi/board.h"

uint64_t rv_perft(const rv_board *b, int depth);
uint64_t rv_perft_divide(const rv_board *b, int depth, uint64_t out[RV_SQUARES]);

#endif
