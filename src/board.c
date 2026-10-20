#include "reversi/board.h"

#include <stdio.h>

#include "reversi/movegen.h"

void rv_board_init(rv_board *b) {
    b->own = RV_BB(rv_sq(4, 3)) | RV_BB(rv_sq(3, 4));
    b->opp = RV_BB(rv_sq(3, 3)) | RV_BB(rv_sq(4, 4));
    b->side = RV_BLACK;
}

rv_bb rv_board_moves(const rv_board *b) { return rv_legal_moves(b->own, b->opp); }

static void swap_sides(rv_board *b) {
    rv_bb t = b->own;
    b->own = b->opp;
    b->opp = t;
    b->side = (b->side == RV_BLACK) ? RV_WHITE : RV_BLACK;
}

bool rv_board_play(rv_board *b, int sq) {
    if (sq == RV_PASS) {
        if (rv_board_moves(b) != 0) {
            return false;
        }
        swap_sides(b);
        return true;
    }
    if (sq < 0 || sq >= RV_SQUARES || ((b->own | b->opp) & RV_BB(sq))) {
        return false;
    }
    rv_bb flips = rv_flips(b->own, b->opp, sq);
    if (flips == 0) {
        return false;
    }
    b->own |= flips | RV_BB(sq);
    b->opp &= ~flips;
    swap_sides(b);
    return true;
}

void rv_board_pass(rv_board *b) { swap_sides(b); }

bool rv_board_is_over(const rv_board *b) {
    return rv_legal_moves(b->own, b->opp) == 0 && rv_legal_moves(b->opp, b->own) == 0;
}

rv_bb rv_board_bits(const rv_board *b, rv_color c) { return (c == b->side) ? b->own : b->opp; }

int rv_board_count(const rv_board *b, rv_color c) { return rv_popcount(rv_board_bits(b, c)); }

int rv_board_empties(const rv_board *b) { return RV_SQUARES - rv_popcount(b->own | b->opp); }

void rv_board_format(const rv_board *b, char *out, size_t cap) {
    rv_bb black = rv_board_bits(b, RV_BLACK);
    rv_bb white = rv_board_bits(b, RV_WHITE);
    rv_bb moves = rv_board_moves(b);
    size_t n = 0;
    n += (size_t)snprintf(out + n, cap - n, "  a b c d e f g h\n");
    for (int rank = 0; rank < 8; rank++) {
        n += (size_t)snprintf(out + n, cap - n, "%d", rank + 1);
        for (int file = 0; file < 8; file++) {
            rv_bb bit = RV_BB(rv_sq(file, rank));
            char c = (black & bit) ? 'X' : (white & bit) ? 'O' : (moves & bit) ? '.' : '-';
            n += (size_t)snprintf(out + n, cap - n, " %c", c);
        }
        n += (size_t)snprintf(out + n, cap - n, "\n");
    }
    snprintf(out + n, cap - n, "%s to move  X:%d O:%d\n", b->side == RV_BLACK ? "X" : "O",
             rv_popcount(black), rv_popcount(white));
}
