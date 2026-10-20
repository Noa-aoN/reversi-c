# reversi-c

[![ci](https://github.com/Noa-aoN/reversi-c/actions/workflows/ci.yml/badge.svg)](https://github.com/Noa-aoN/reversi-c/actions/workflows/ci.yml)

A Reversi (Othello) engine in portable C11. No dependencies, no heap allocation in the engine,
and every piece of game logic is pinned down by perft.

[日本語](#日本語)

## Status

| Stage | Scope | State |
|---|---|---|
| 1 | Bitboard, move generation, perft | done |
| 2 | CLI: human vs human / random / greedy | planned |
| 3 | Negamax + alpha-beta, iterative deepening, evaluation | planned |
| 4 | Endgame exact solver, transposition table, time control | planned |
| 5 | Benchmarks and self-play results | planned |

## Design constraints

- C11, `-Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion`, zero third-party code.
- The engine (`src/*.c`) never calls `malloc`. State is a 24-byte struct; tables are fixed size.
- Board is two `uint64_t` bitboards from the side-to-move's point of view. Move generation and
  flipping are shift-and-mask flood passes over 8 directions, with no per-square loop.
- Tests are a single header (`tests/minitest.h`); CI runs gcc and clang, each with and without
  ASan/UBSan, plus valgrind.

## Build and run

```sh
make            # build/reversi
make test       # unit tests + perft(0..9)
make asan       # same tests under AddressSanitizer + UBSan
make bench      # perft(10) throughput
./build/reversi show
./build/reversi perft 9
```

## Correctness: perft

`perft(d)` counts leaf nodes `d` plies from the initial position. A forced pass counts as one ply;
a finished game is one leaf. Results match the published reference values.

| depth | leaves |
|---:|---:|
| 1 | 4 |
| 2 | 12 |
| 3 | 56 |
| 4 | 244 |
| 5 | 1,396 |
| 6 | 8,200 |
| 7 | 55,092 |
| 8 | 390,216 |
| 9 | 3,005,288 |
| 10 | 24,571,284 |

Depths 1 to 9 are asserted in `make test`; depth 10 is `make bench`
(about 0.9 s, 26 M leaves/s on an Apple M-series core, `-O2`).

## Layout

```
include/reversi/   public headers (bitboard, movegen, board, perft)
src/               engine
src/cli/           command-line front end
tests/             minitest.h + test_*.c
bench/             perft throughput
docs/spec.md       design notes and decisions
```

## License

MIT

---

## 日本語

移植性のある C11 で書いた Reversi (オセロ) エンジン。外部依存なし、エンジン内で `malloc` を使わず、
ゲームロジックの正しさは perft で機械的に検証している。

### 設計上の制約

- C11、`-Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion`、サードパーティコードなし。
- エンジン (`src/*.c`) は `malloc` を呼ばない。局面は 24 バイトの構造体、テーブルは固定長。
- 盤面は手番側から見た 2 本の `uint64_t` bitboard。合法手生成と反転は 8 方向の
  shift + mask の flood 走査で、マスごとのループを持たない。
- テストはヘッダ 1 つ (`tests/minitest.h`) で完結。CI は gcc / clang それぞれで
  通常ビルドと ASan/UBSan ビルド、加えて valgrind を回す。

### 正しさの根拠

`perft(d)` は初期局面から `d` 手先の葉ノード数。パスは 1 手として数え、終局は 1 葉とする。
1〜9 手は `make test` で、10 手は `make bench` で既知の参照値と一致することを確認している。

### 進め方

段階ごとに「テストが通る状態」を保ったまま積み上げる (上の Status 表を参照)。
設計判断の経緯は `docs/spec.md` に残す。
