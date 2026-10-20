#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "reversi/board.h"
#include "reversi/perft.h"

static int usage(void) {
    fprintf(stderr, "usage: reversi show\n"
                    "       reversi perft <depth>\n");
    return 2;
}

static int cmd_show(void) {
    rv_board b;
    char buf[RV_BOARD_STR_CAP];
    rv_board_init(&b);
    rv_board_format(&b, buf, sizeof buf);
    fputs(buf, stdout);
    return 0;
}

static int cmd_perft(int depth) {
    rv_board b;
    rv_board_init(&b);
    for (int d = 1; d <= depth; d++) {
        clock_t t0 = clock();
        uint64_t n = rv_perft(&b, d);
        double sec = (double)(clock() - t0) / CLOCKS_PER_SEC;
        printf("perft(%2d) = %12llu  %8.3fs\n", d, (unsigned long long)n, sec);
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "show") == 0) {
        return cmd_show();
    }
    if (argc == 3 && strcmp(argv[1], "perft") == 0) {
        int depth = atoi(argv[2]);
        if (depth < 1 || depth > 20) {
            return usage();
        }
        return cmd_perft(depth);
    }
    return usage();
}
