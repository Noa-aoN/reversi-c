#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "reversi/perft.h"

int main(int argc, char **argv) {
    int depth = (argc > 1) ? atoi(argv[1]) : 10;
    rv_board b;
    rv_board_init(&b);
    clock_t t0 = clock();
    uint64_t leaves = rv_perft(&b, depth);
    double sec = (double)(clock() - t0) / CLOCKS_PER_SEC;
    printf("perft(%d) = %llu leaves in %.3fs (%.1f M leaves/s)\n", depth,
           (unsigned long long)leaves, sec, (double)leaves / sec / 1e6);
    return 0;
}
