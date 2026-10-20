#ifndef REVERSI_MOVEGEN_H
#define REVERSI_MOVEGEN_H

#include "reversi/bitboard.h"

rv_bb rv_legal_moves(rv_bb own, rv_bb opp);
rv_bb rv_flips(rv_bb own, rv_bb opp, int sq);

#endif
