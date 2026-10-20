#ifndef REVERSI_PERFT_H
#define REVERSI_PERFT_H

#include <stdint.h>

#include "reversi/board.h"

uint64_t rv_perft(const rv_board *b, int depth);

#endif
