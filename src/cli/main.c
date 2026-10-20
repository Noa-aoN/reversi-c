#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "reversi/board.h"
#include "reversi/perft.h"
#include "reversi/player.h"
#include "reversi/search.h"

typedef enum player_kind { HUMAN, RANDOM, GREEDY, AB, TIMED } player_kind;

typedef struct player {
    player_kind kind;
    int depth;
    double seconds;
} player;

#define MAX_PLIES 128
#define DEFAULT_DEPTH 6
#define DEFAULT_MS 1000

static int usage(void) {
    fprintf(stderr,
            "usage: reversi play [black] [white] [seed]      player: human|random|greedy|ab[N]|t[MS]\n"
            "       reversi match <a> <b> <games> [seed]     self-play, colors alternate\n"
            "       reversi replay <moves>                   e.g. f5d6c3d3c4\n"
            "       reversi show\n"
            "       reversi perft <depth>\n");
    return 2;
}

static int parse_player(const char *s, player *out) {
    if (strcmp(s, "human") == 0) {
        out->kind = HUMAN;
    } else if (strcmp(s, "random") == 0) {
        out->kind = RANDOM;
    } else if (strcmp(s, "greedy") == 0) {
        out->kind = GREEDY;
    } else if (strncmp(s, "ab", 2) == 0) {
        out->kind = AB;
        out->depth = s[2] ? atoi(s + 2) : DEFAULT_DEPTH;
        if (out->depth < 1 || out->depth > RV_SEARCH_MAX_DEPTH) {
            return 0;
        }
    } else if (s[0] == 't') {
        int ms = s[1] ? atoi(s + 1) : DEFAULT_MS;
        if (ms < 1) {
            return 0;
        }
        out->kind = TIMED;
        out->depth = RV_SEARCH_MAX_DEPTH;
        out->seconds = ms / 1000.0;
    } else {
        return 0;
    }
    return 1;
}

static void describe(const player *p, char *out, size_t cap) {
    static const char *names[] = {"human", "random", "greedy", "ab", "t"};
    if (p->kind == AB) {
        snprintf(out, cap, "ab%d", p->depth);
    } else if (p->kind == TIMED) {
        snprintf(out, cap, "t%d", (int)(p->seconds * 1000.0 + 0.5));
    } else {
        snprintf(out, cap, "%s", names[p->kind]);
    }
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

static int pick_move(const player *p, const rv_board *b, rv_rng *rng, int verbose) {
    rv_search_result res;
    switch (p->kind) {
    case HUMAN:
        return read_human_move(b);
    case RANDOM:
        return rv_pick_random(b, rng);
    case GREEDY:
        return rv_pick_greedy(b);
    default:
        if (p->kind == TIMED) {
            rv_search_timed(b, p->depth, p->seconds, &res);
        } else {
            rv_search_id(b, p->depth, &res);
        }
        if (verbose) {
            printf("depth %d%s  score %+d  nodes %llu  %.2fs  %.1f Mn/s\n", res.depth,
                   res.exact ? " (exact)" : "", res.score, (unsigned long long)res.nodes,
                   res.seconds, res.seconds > 0 ? (double)res.nodes / res.seconds / 1e6 : 0.0);
        }
        return res.move;
    }
}

static int cmd_play(const player players[2], uint64_t seed) {
    char names[2][16];
    rv_board b;
    rv_rng rng;
    int history[MAX_PLIES];
    int n = 0;
    describe(&players[RV_BLACK], names[0], sizeof names[0]);
    describe(&players[RV_WHITE], names[1], sizeof names[1]);
    rv_board_init(&b);
    rv_rng_seed(&rng, seed);
    printf("X: %s  O: %s  (type a square like f5, or quit)\n", names[0], names[1]);
    while (!rv_board_is_over(&b) && n < MAX_PLIES) {
        print_board(&b);
        if (rv_board_moves(&b) == 0) {
            printf("%s has no moves, pass\n", b.side == RV_BLACK ? "X" : "O");
            rv_board_pass(&b);
            continue;
        }
        int sq = pick_move(&players[b.side], &b, &rng, 1);
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

static int cmd_match(const player *a, const player *bp, int games, uint64_t seed) {
    char na[16], nb[16];
    rv_rng rng;
    int wins = 0, draws = 0, losses = 0;
    int discs_a = 0, discs_b = 0;
    describe(a, na, sizeof na);
    describe(bp, nb, sizeof nb);
    rv_rng_seed(&rng, seed);
    for (int g = 0; g < games; g++) {
        rv_board b;
        rv_color a_color = (g % 2 == 0) ? RV_BLACK : RV_WHITE;
        rv_board_init(&b);
        for (int i = 0; i < 4 && !rv_board_is_over(&b); i++) {
            rv_board_play(&b, rv_pick_random(&b, &rng));
        }
        while (!rv_board_is_over(&b)) {
            if (rv_board_moves(&b) == 0) {
                rv_board_pass(&b);
                continue;
            }
            const player *p = (b.side == a_color) ? a : bp;
            rv_board_play(&b, pick_move(p, &b, &rng, 0));
        }
        int ca = rv_board_count(&b, a_color);
        int cb = rv_board_count(&b, a_color == RV_BLACK ? RV_WHITE : RV_BLACK);
        discs_a += ca;
        discs_b += cb;
        if (ca > cb) {
            wins++;
        } else if (ca < cb) {
            losses++;
        } else {
            draws++;
        }
    }
    printf("%s vs %s: %d games  W %d  D %d  L %d  (%.1f%%)  discs %d:%d\n", na, nb, games, wins,
           draws, losses, (wins + 0.5 * draws) * 100.0 / games, discs_a, discs_b);
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
        player players[2] = {{HUMAN, 0, 0.0}, {AB, DEFAULT_DEPTH, 0.0}};
        if (argc >= 3 && !parse_player(argv[2], &players[RV_BLACK])) {
            return usage();
        }
        if (argc >= 4 && !parse_player(argv[3], &players[RV_WHITE])) {
            return usage();
        }
        uint64_t seed = (argc == 5) ? strtoull(argv[4], NULL, 10) : (uint64_t)time(NULL);
        return cmd_play(players, seed);
    }
    if (strcmp(argv[1], "match") == 0 && (argc == 5 || argc == 6)) {
        player a, b;
        int games = atoi(argv[4]);
        if (!parse_player(argv[2], &a) || !parse_player(argv[3], &b) || games < 1 ||
            a.kind == HUMAN || b.kind == HUMAN) {
            return usage();
        }
        uint64_t seed = (argc == 6) ? strtoull(argv[5], NULL, 10) : (uint64_t)time(NULL);
        return cmd_match(&a, &b, games, seed);
    }
    return usage();
}
