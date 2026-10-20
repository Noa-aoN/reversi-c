#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "reversi/board.h"
#include "reversi/perft.h"
#include "reversi/player.h"

typedef enum player_kind { HUMAN, RANDOM, GREEDY } player_kind;

#define MAX_PLIES 128

static int usage(void) {
    fprintf(stderr, "usage: reversi play [black] [white] [seed]   player: human|random|greedy\n"
                    "       reversi replay <moves>              e.g. f5d6c3d3c4\n"
                    "       reversi show\n"
                    "       reversi perft <depth>\n");
    return 2;
}

static void print_board(const rv_board *b) {
    char buf[RV_BOARD_STR_CAP];
    rv_board_format(b, buf, sizeof buf);
    fputs(buf, stdout);
}

static void print_transcript(const int *moves, int n) {
    char sq[3];
    fputs("moves: ", stdout);
    for (int i = 0; i < n; i++) {
        rv_sq_to_str(moves[i], sq);
        fputs(sq, stdout);
    }
    fputc('\n', stdout);
}

static void print_result(const rv_board *b) {
    int black = rv_board_count(b, RV_BLACK);
    int white = rv_board_count(b, RV_WHITE);
    printf("game over  X:%d O:%d  %s\n", black, white,
           black > white ? "X wins" : white > black ? "O wins" : "draw");
}

static int read_human_move(const rv_board *b) {
    char line[32];
    rv_bb moves = rv_board_moves(b);
    for (;;) {
        char sq[3];
        rv_bb m = moves;
        fputs("legal:", stdout);
        while (m) {
            rv_sq_to_str(rv_ctz(m), sq);
            printf(" %s", sq);
            m &= m - 1;
        }
        fputs("\n> ", stdout);
        fflush(stdout);
        if (fgets(line, sizeof line, stdin) == NULL) {
            return -2;
        }
        line[strcspn(line, "\r\n")] = '\0';
        if (strcmp(line, "quit") == 0 || strcmp(line, "q") == 0) {
            return -2;
        }
        int sq_idx = rv_sq_from_str(line);
        if (sq_idx >= 0 && (moves & RV_BB(sq_idx))) {
            return sq_idx;
        }
        puts("illegal move");
    }
}

static int parse_kind(const char *s, player_kind *out) {
    if (strcmp(s, "human") == 0) {
        *out = HUMAN;
    } else if (strcmp(s, "random") == 0) {
        *out = RANDOM;
    } else if (strcmp(s, "greedy") == 0) {
        *out = GREEDY;
    } else {
        return 0;
    }
    return 1;
}

static int cmd_play(player_kind kinds[2], uint64_t seed) {
    static const char *names[] = {"human", "random", "greedy"};
    rv_board b;
    rv_rng rng;
    int history[MAX_PLIES];
    int n = 0;
    rv_board_init(&b);
    rv_rng_seed(&rng, seed);
    printf("X: %s  O: %s  (type a square like f5, or quit)\n", names[kinds[RV_BLACK]],
           names[kinds[RV_WHITE]]);
    while (!rv_board_is_over(&b) && n < MAX_PLIES) {
        print_board(&b);
        if (rv_board_moves(&b) == 0) {
            printf("%s has no moves, pass\n", b.side == RV_BLACK ? "X" : "O");
            rv_board_pass(&b);
            continue;
        }
        int sq;
        switch (kinds[b.side]) {
        case HUMAN:
            sq = read_human_move(&b);
            break;
        case RANDOM:
            sq = rv_pick_random(&b, &rng);
            break;
        default:
            sq = rv_pick_greedy(&b);
            break;
        }
        if (sq == -2) {
            print_transcript(history, n);
            return 0;
        }
        char sqs[3];
        rv_sq_to_str(sq, sqs);
        printf("%s plays %s\n\n", b.side == RV_BLACK ? "X" : "O", sqs);
        rv_board_play(&b, sq);
        history[n++] = sq;
    }
    print_board(&b);
    print_result(&b);
    print_transcript(history, n);
    return 0;
}

static int cmd_replay(const char *moves) {
    rv_board b;
    size_t len = strlen(moves);
    rv_board_init(&b);
    if (len % 2 != 0) {
        fprintf(stderr, "moves must be pairs like f5d6\n");
        return 1;
    }
    for (size_t i = 0; i < len; i += 2) {
        char sq[3] = {moves[i], moves[i + 1], '\0'};
        int idx = rv_sq_from_str(sq);
        if (rv_board_moves(&b) == 0 && !rv_board_is_over(&b)) {
            rv_board_pass(&b);
        }
        if (idx < 0 || !rv_board_play(&b, idx)) {
            fprintf(stderr, "illegal move %s at ply %zu\n", sq, i / 2 + 1);
            print_board(&b);
            return 1;
        }
    }
    print_board(&b);
    if (rv_board_is_over(&b)) {
        print_result(&b);
    }
    return 0;
}

static int cmd_show(void) {
    rv_board b;
    rv_board_init(&b);
    print_board(&b);
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
    if (argc < 2) {
        return usage();
    }
    if (strcmp(argv[1], "show") == 0 && argc == 2) {
        return cmd_show();
    }
    if (strcmp(argv[1], "perft") == 0 && argc == 3) {
        int depth = atoi(argv[2]);
        return (depth < 1 || depth > 20) ? usage() : cmd_perft(depth);
    }
    if (strcmp(argv[1], "replay") == 0 && argc == 3) {
        return cmd_replay(argv[2]);
    }
    if (strcmp(argv[1], "play") == 0 && argc <= 5) {
        player_kind kinds[2] = {HUMAN, GREEDY};
        if (argc >= 3 && !parse_kind(argv[2], &kinds[RV_BLACK])) {
            return usage();
        }
        if (argc >= 4 && !parse_kind(argv[3], &kinds[RV_WHITE])) {
            return usage();
        }
        uint64_t seed = (argc == 5) ? strtoull(argv[4], NULL, 10) : (uint64_t)time(NULL);
        return cmd_play(kinds, seed);
    }
    return usage();
}
