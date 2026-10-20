#ifndef REVERSI_EVAL_H
#define REVERSI_EVAL_H

#include "reversi/board.h"

#define RV_SCORE_INF 1000000
#define RV_SCORE_DISC 1000

int rv_eval(const rv_board *b);
int rv_eval_final(const rv_board *b);

#endif
