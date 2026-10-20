#include "reversi/tt.h"

static uint64_t mix(uint64_t x) {
    x ^= x >> 30;
    x *= UINT64_C(0xBF58476D1CE4E5B9);
    x ^= x >> 27;
    x *= UINT64_C(0x94D049BB133111EB);
    x ^= x >> 31;
    return x;
}

uint64_t rv_hash(const rv_board *b) {
    return mix(b->own ^ UINT64_C(0x9E3779B97F4A7C15)) ^ mix(b->opp * UINT64_C(0xD6E8FEB86659FD93) + 1);
}

bool rv_tt_init(rv_tt *tt, rv_tt_entry *storage, size_t count) {
    if (storage == NULL || count == 0 || (count & (count - 1)) != 0) {
        return false;
    }
    tt->entries = storage;
    tt->mask = count - 1;
    rv_tt_clear(tt);
    return true;
}

void rv_tt_clear(rv_tt *tt) {
    for (size_t i = 0; i <= tt->mask; i++) {
        tt->entries[i].used = 0;
    }
}

const rv_tt_entry *rv_tt_probe(const rv_tt *tt, const rv_board *b, uint64_t hash) {
    const rv_tt_entry *e = &tt->entries[hash & tt->mask];
    if (e->used && e->own == b->own && e->opp == b->opp) {
        return e;
    }
    return NULL;
}

void rv_tt_store(rv_tt *tt, const rv_board *b, uint64_t hash, int depth, int score, int flag,
                 int move) {
    rv_tt_entry *e = &tt->entries[hash & tt->mask];
    bool same = e->used && e->own == b->own && e->opp == b->opp;
    if (same && e->depth > depth) {
        return;
    }
    e->own = b->own;
    e->opp = b->opp;
    e->score = score;
    e->depth = (int8_t)depth;
    e->flag = (int8_t)flag;
    e->move = (int8_t)move;
    e->used = 1;
}
