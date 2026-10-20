# reversi-c

[![ci](https://github.com/Noa-aoN/reversi-c/actions/workflows/ci.yml/badge.svg)](https://github.com/Noa-aoN/reversi-c/actions/workflows/ci.yml)

移植性のある C11 で書いた Reversi (オセロ) エンジンと端末用の対局 CLI。
外部依存なし、エンジン内で `malloc` を使わず、ゲームロジックの正しさは perft で機械的に検証している。

[English](#english)

## 進捗

| 段階 | 内容 | 状態 |
|---|---|---|
| 1 | bitboard、合法手生成、perft | 済 |
| 2 | CLI 対局 (人 / random / greedy)、棋譜の出力と再生 | 済 |
| 3 | negamax + αβ、反復深化、評価関数 | 予定 |
| 4 | 終盤完全読み、置換表、時間制御 | 予定 |
| 5 | ベンチマークと自己対戦の数値 | 予定 |

## ビルドと遊び方

```sh
make                                  # build/reversi
./build/reversi play                  # 自分 (黒 X) vs greedy (白 O)
./build/reversi play human human      # 人 vs 人
./build/reversi play random greedy 42 # AI 同士。第 3 引数は乱数シード
./build/reversi replay f5d6c3d3c4     # 棋譜を再生して局面を表示
./build/reversi show                  # 初期局面と合法手
./build/reversi perft 9
```

対局中は `f5` のようにマス目を入力する。`.` が合法手、`X` が黒、`O` が白。`quit` で中断。
打てる手がない側は自動でパスする。終局時に勝敗と棋譜 (`moves: f5d6c3...`) を表示し、
その棋譜は `replay` にそのまま渡せる。

```
  a b c d e f g h
1 - - - - - - - -
2 - - - - - - - -
3 - - - . - - - -
4 - - . O X - - -
5 - - - X O . - -
6 - - - - . - - -
7 - - - - - - - -
8 - - - - - - - -
X to move  X:2 O:2
legal: d3 c4 f5 e6
> f5
```

```sh
make test       # 単体テスト + perft(0..9)
make asan       # 同じテストを AddressSanitizer + UBSan で
make bench      # perft(10) のスループット
```

## 設計上の制約

- C11、`-Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion`、サードパーティコードなし。
- エンジン (`src/*.c`) は `malloc` を呼ばない。局面は 24 バイトの構造体、テーブルは固定長。
- 盤面は手番側から見た 2 本の `uint64_t` bitboard。合法手生成と反転は 8 方向の
  shift + mask の flood 走査で、マスごとのループを持たない。
- テストはヘッダ 1 つ (`tests/minitest.h`) で完結。CI は gcc / clang それぞれで
  通常ビルドと ASan/UBSan ビルド、加えて valgrind を回す。

## 正しさの根拠: perft

`perft(d)` は初期局面から `d` 手先の葉ノード数。パスは 1 手として数え、終局は 1 葉とする。
公開されている参照値と一致する。

| 手数 | 葉ノード数 |
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

1〜9 手は `make test` で検証し、10 手は `make bench` で計測する
(Apple M 系 1 コア、`-O2` で約 0.9 秒、26M leaves/s)。

## 構成

```
include/reversi/   公開ヘッダ (bitboard, movegen, board, perft, player)
src/               エンジン
src/cli/           コマンドラインのフロントエンド
tests/             minitest.h + test_*.c
bench/             perft スループット
docs/spec.md       設計メモと判断の経緯
```

## ライセンス

MIT

---

## English

A Reversi (Othello) engine in portable C11 with a terminal front end. No dependencies, no heap
allocation in the engine, and every piece of game logic is pinned down by perft.

### Status

| Stage | Scope | State |
|---|---|---|
| 1 | Bitboard, move generation, perft | done |
| 2 | CLI play (human / random / greedy), transcript output and replay | done |
| 3 | Negamax + alpha-beta, iterative deepening, evaluation | planned |
| 4 | Endgame exact solver, transposition table, time control | planned |
| 5 | Benchmarks and self-play results | planned |

### Build and play

```sh
make                                  # build/reversi
./build/reversi play                  # you (black, X) vs greedy (white, O)
./build/reversi play human human
./build/reversi play random greedy 42 # engine vs engine; third arg is the RNG seed
./build/reversi replay f5d6c3d3c4     # replay a transcript and show the position
./build/reversi show                  # initial position with legal moves
./build/reversi perft 9
```

Enter squares like `f5`. `.` marks legal moves, `X` is black, `O` is white, `quit` stops the game.
A side with no legal move passes automatically. At the end the result and the transcript
(`moves: f5d6c3...`) are printed; the transcript can be fed straight back into `replay`.

```sh
make test       # unit tests + perft(0..9)
make asan       # same tests under AddressSanitizer + UBSan
make bench      # perft(10) throughput
```

### Design constraints

- C11, `-Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion`, zero third-party code.
- The engine (`src/*.c`) never calls `malloc`. State is a 24-byte struct; tables are fixed size.
- Board is two `uint64_t` bitboards from the side-to-move's point of view. Move generation and
  flipping are shift-and-mask flood passes over 8 directions, with no per-square loop.
- Tests are a single header (`tests/minitest.h`); CI runs gcc and clang, each with and without
  ASan/UBSan, plus valgrind.

### Correctness: perft

`perft(d)` counts leaf nodes `d` plies from the initial position. A forced pass counts as one ply;
a finished game is one leaf. Depths 1 to 10 match the published reference values
(4, 12, 56, 244, 1396, 8200, 55092, 390216, 3005288, 24571284). Depths 1 to 9 are asserted in
`make test`; depth 10 is `make bench` (about 0.9 s, 26 M leaves/s on an Apple M-series core, `-O2`).

### Layout

```
include/reversi/   public headers (bitboard, movegen, board, perft, player)
src/               engine
src/cli/           command-line front end
tests/             minitest.h + test_*.c
bench/             perft throughput
docs/spec.md       design notes and decisions
```

### License

MIT
