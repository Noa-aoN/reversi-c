#ifndef MINITEST_H
#define MINITEST_H

#include <inttypes.h>
#include <stdio.h>

static int mt_checks = 0;
static int mt_fails = 0;

#define MT_CHECK(cond)                                                                             \
    do {                                                                                           \
        mt_checks++;                                                                               \
        if (!(cond)) {                                                                             \
            mt_fails++;                                                                            \
            fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                      \
        }                                                                                          \
    } while (0)

#define MT_CHECK_EQ_U64(got, want)                                                                 \
    do {                                                                                           \
        uint64_t mt_g = (uint64_t)(got);                                                           \
        uint64_t mt_w = (uint64_t)(want);                                                          \
        mt_checks++;                                                                               \
        if (mt_g != mt_w) {                                                                        \
            mt_fails++;                                                                            \
            fprintf(stderr, "  FAIL %s:%d: %s == %" PRIu64 ", want %" PRIu64 "\n", __FILE__,      \
                    __LINE__, #got, mt_g, mt_w);                                                   \
        }                                                                                          \
    } while (0)

#define MT_RUN(fn)                                                                                 \
    do {                                                                                           \
        printf("- %s\n", #fn);                                                                     \
        fn();                                                                                      \
    } while (0)

static inline int mt_report(const char *suite) {
    printf("%s: %d checks, %d failures\n", suite, mt_checks, mt_fails);
    return mt_fails == 0 ? 0 : 1;
}

#endif
