#include "reversi/bitboard.h"

#include <stddef.h>

int rv_sq_from_str(const char *s) {
    if (s == NULL || s[0] == '\0' || s[1] == '\0') {
        return -1;
    }
    int file = s[0] - 'a';
    if (file < 0 || file > 7) {
        file = s[0] - 'A';
    }
    int rank = s[1] - '1';
    if (file < 0 || file > 7 || rank < 0 || rank > 7 || s[2] != '\0') {
        return -1;
    }
    return rv_sq(file, rank);
}

void rv_sq_to_str(int sq, char out[3]) {
    out[0] = (char)('a' + rv_sq_file(sq));
    out[1] = (char)('1' + rv_sq_rank(sq));
    out[2] = '\0';
}
