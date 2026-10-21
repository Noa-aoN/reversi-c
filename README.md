# reversi-c

[![ci](https://github.com/Noa-aoN/reversi-c/actions/workflows/ci.yml/badge.svg)](https://github.com/Noa-aoN/reversi-c/actions/workflows/ci.yml)

C11 で書いた Reversi (オセロ) のエンジンと、端末で対局するための CLI です。
外部ライブラリなし、エンジン内でのヒープ確保なし、正しさは perft と参照探索との一致で機械的に確かめています。

[English](#english)

## 何を作ったか

盤面の表現から探索・評価・置換表・時間制御までを一通り持つ、対局できる Reversi エンジンです。
組み込みで C を書いてきた流儀をそのまま持ち込んでいて、守っている約束は 3 つです。

- 依存を持たない。標準 C ライブラリだけでビルドできる。
- エンジン (`src/*.c`) は `malloc` を呼ばない。局面は 24 バイトの構造体で、表はすべて固定長。
- 正しさを目視ではなく機械で確かめる。合法手は perft の参照値と、探索は枝刈りなしの参照実装と一致させる。

UI は意図的に端末だけにしています。見せたいのはエンジンの中身なので、画面に手間をかけるより
自己対戦やベンチのような機械的な検証に寄せました。

## まず遊ぶ

```sh
make
./build/reversi play                  # 自分が黒 (X)、相手は 6 手読みの AI
./build/reversi play human t500       # 相手は 1 手 500 ms 考える AI
./build/reversi play human human      # 人どうし
./build/reversi match ab6 greedy 40 1 # AI どうしを 40 局 (先後交互、序盤 4 手はランダム)
./build/reversi replay f5d6c3d3c4     # 棋譜を再生して局面を表示
```

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

`X` が黒、`O` が白、`.` が打てる場所です。`f5` のように入力し、`quit` で中断します。
打てる手がない側は自動でパスします。終局すると勝敗と棋譜 (`moves: f5d6c3...`) が出て、
その棋譜はそのまま `replay` に渡せます。

AI の種類は `random` / `greedy` (反転数が最大の手) / `ab[N]` (N 手読み、既定 6) /
`t[MS]` (1 手 MS ミリ秒、既定 1000) です。AI は手ごとに深さ・評価値・ノード数・速度を表示し、
終盤で読み切ると `(exact)` が付きます。

## 中身の流れ

### 1. 盤面は 64 ビット整数 2 本

64 マスを 1 ビットずつ、手番側の石 `own` と相手の石 `opp` の 2 本の `uint64_t` で持ちます (bitboard)。
手を打ったら 2 本を入れ替えるだけで手番が変わるので、探索は色で分岐しません。
局面のコピーが 24 バイトで済むため、探索中は undo を実装せず値コピーで進めます。

### 2. 合法手は「シフトして流す」だけ

ある方向に相手の石が続き、その先に自分の石がある、という条件を 8 方向ぶん調べると合法手になります。
これをマスごとのループではなく、bitboard 全体を 1, 2, 4 マスずつシフトして相手石の上を流す
(Kogge-Stone 充填) 方法で求めます。相手石の列は最長 6 個なので 3 回のシフトで届きます。
盤の端をまたがないように、シフト幅ぶんの列をマスクで落としています。

### 3. 正しさは perft で固定する

`perft(d)` は初期局面から d 手先にある局面の数です。合法手生成や反転にバグがあると必ずどこかの
数が変わるので、公開されている参照値と一致すればロジック全体が正しいと言えます。

| 手数 | 局面数 |
|---:|---:|
| 1 | 4 |
| 5 | 1,396 |
| 8 | 390,216 |
| 9 | 3,005,288 |
| 10 | 24,571,284 |

1〜9 手は `make test` で毎回照合し、10 手は `make bench` で速度も測ります。
パスは 1 手と数え、終局は 1 局面と数える定義で、10 手目はこの定義がはじめて効く深さです
(最短の終局が 9 手なので)。ここが一致することで定義も固定されます。

### 4. 探索は negamax + αβ、終盤は自然に読み切り

深さ優先で手を読み、αβ 枝刈りで無駄な枝を落とします。読む順番は角 → 普通のマス → C → X の
4 段階で、良さそうな手から読むほど枝刈りが効きます。反復深化で 1 手読みから順に深くし、
前回の最善手を次の深さで最初に読みます。
読む深さは空きマスの数で頭打ちにしているので、終盤は特別なコードなしに石差で読み切ります。

### 5. 評価は角と着手可能数

静的評価は「角 100 / X -50 / C -20 / ...」の重みをマスの種類ごとの bitmask に掛けた合計と、
着手可能数の差です。角が埋まった後はその隣の X・C マスの罰則を外します (角を取られた後の X は
もう危険ではないため)。終局は石差 x 1000 で、静的評価を必ず上回るようにしています。

### 6. 置換表は「同じ深さだけ」使う

一度読んだ局面の結果を 2^18 エントリの静的配列に覚えます。エントリには局面そのものを入れて
完全一致で照合するため、ハッシュ衝突で別の局面の値を返すことがありません。
値として使うのは同じ深さで読んだ結果だけにしています。深い結果を使えばもう少し強くなりますが、
そうすると固定深さの評価値が表の中身で変わり、参照実装との一致テストが成り立たなくなるためです。
手順前後の候補にはどの深さの結果も使います。

### 7. 時間制御

`t[MS]` は反復深化を回しながら 4096 ノードごとに時計を見て、締切を過ぎたら探索を打ち切り、
直前に読み終えた深さの結果を使います。打ち切った途中の結果は置換表に書きません。
時計はエンジンが持たず、ホストが `rv_search_set_clock` で関数を渡します (CLI は `timespec_get`、
既定は C89 の `clock()`)。マイコンでは SysTick を読む関数を渡すだけで済みます。

## 強さと速さ

`match` による自己対戦 (40 局、seed 1、先後交互、序盤 4 手ランダム)。左側から見た勝率です。

| 対戦 | 勝率 | 石数 |
|---|---:|---:|
| ab4 vs greedy | 100% | 2088 : 453 |
| ab6 vs greedy | 100% | 2184 : 340 |
| ab6 vs ab4 | 75.0% | 1634 : 916 |
| ab8 vs ab6 (20 局) | 67.5% | 765 : 514 |
| t200 vs ab6 (10 局) | 90.0% | 463 : 176 |

深く読むほど勝つ、という順序が崩れていないことを確認しています。

速度 (Apple M 系 1 コア、`-O2`、`make bench`):

| 項目 | 値 |
|---|---:|
| perft(10) | 48 M 局面/秒 (Kogge-Stone 化で 26 M から改善) |
| 探索 (8 手読み x 20 局面) | 12 M ノード/秒 |
| ab8 vs ab6 の 20 局 | 19.1 秒 → 3.1 秒 (置換表 + 合法手生成の改善) |

エンジン部分を Cortex-M4 向けに `-Os` でビルドすると、コードは 5.0 KB、RAM は置換表込みで 25 KB です
(`RV_TT_BITS=10`、CI の `cross` ジョブで毎回計測)。

## どう確かめているか

```sh
make test       # 単体テスト + perft(0..9) + 参照探索との一致
make smoke      # CLI の各サブコマンドを 1 回ずつ通す
make asan       # 同じテストを AddressSanitizer + UBSan で
make bench      # perft(10) と探索のスループット
make cross      # arm-none-eabi-gcc で Cortex-M4 向けにエンジンをビルドしサイズを表示
```

CI は gcc と clang でそれぞれ通常ビルドと ASan/UBSan ビルドを回し、さらに valgrind でも全テストを通します。
静的解析 (cppcheck と clang の scan-build) も毎回かけ、指摘ゼロを維持しています。
Cortex-M4 向けのクロスビルドも CI に入れていて、「マイコンに載る」が言葉だけにならないようにしています。
テストはヘッダ 1 つの自作フレームワーク (`tests/minitest.h`) で、外部に依存しません。

## 今後

エンジンは UI も OS も前提にしていないので、そのまま別の場所へ持っていけます。

- マイコンへの移植。Cortex-M4 向けビルドは CI で通っていて、残るのは UART で手をやり取りする薄い層と時計関数だけ。
- WASM にしてブラウザで対局 (別リポで)。
- 標準入出力の対戦プロトコルを付け、他のエンジンや GUI と接続して強さを測る。
- 自己対戦の棋譜から評価関数の重みを学習する。

考察の詳細は [docs/spec.md](docs/spec.md) の「今後の活用」にあります。

## 構成

```
include/reversi/   公開ヘッダ (bitboard, movegen, board, perft, player, eval, search, tt)
src/               エンジン
src/cli/           端末用フロントエンド
tests/             minitest.h + test_*.c
bench/             perft と探索のベンチ
docs/spec.md       設計判断の記録と今後の活用
```

## ライセンス

MIT

---

## English

A Reversi (Othello) engine in C11 with a terminal front end. No dependencies, no heap allocation
inside the engine, and correctness is checked mechanically: move generation against published
perft counts, search against a pruning-free reference implementation.

### Play

```sh
make
./build/reversi play                  # you as black (X) vs a 6-ply engine
./build/reversi play human t500       # engine thinks 500 ms per move
./build/reversi match ab6 greedy 40 1 # 40-game self-play, colors alternate
./build/reversi replay f5d6c3d3c4     # replay a transcript
```

Enter squares like `f5`; `.` marks legal moves; `quit` stops. Players: `random`, `greedy`,
`ab[N]` (N plies, default 6), `t[MS]` (milliseconds per move, default 1000).

### How it works

1. **Board**: two `uint64_t` bitboards, `own` and `opp`, from the side to move. A move swaps
   them, so search never branches on color. A position is 24 bytes and is copied, never undone.
2. **Move generation**: Kogge-Stone occluded fill in 8 directions. Shifts of 1, 2 and 4 cover a
   run of up to 6 opponent stones; wrap-around is masked off per shift width.
3. **Perft**: depths 1 to 9 are asserted in `make test`, depth 10 (24,571,284) in `make bench`. A
   pass is one ply, a finished game one leaf; depth 10 is the first depth where that matters.
4. **Search**: negamax with alpha-beta, move ordering corners / plain / C / X, iterative
   deepening seeded with the previous best move. Depth is capped at the number of empties, so
   the endgame is solved exactly without separate code.
5. **Evaluation**: weighted square-class bitmasks plus mobility; X and C penalties are lifted once
   the adjacent corner is taken. Finished games score disc difference x 1000.
6. **Transposition table**: 2^18 static entries holding the full position, so a collision can
   only miss. Only same-depth entries supply values, which keeps fixed-depth scores independent
   of the table and the reference-search test valid.
7. **Time control**: iterative deepening, clock checked every 4096 nodes, last completed depth
   wins; aborted iterations are not stored. The engine owns no clock: the host passes one with
   `rv_search_set_clock` (the CLI uses `timespec_get`, the default is C89 `clock()`).

### Strength and speed

Self-play (40 games, seed 1, alternating colors, 4 random opening plies): ab4 and ab6 beat greedy
100%, ab6 beats ab4 75%, ab8 beats ab6 67.5% (20 games), t200 beats ab6 90% (10 games).
Perft runs at 48 M leaves/s, search at 12 M nodes/s (Apple M-series core, `-O2`). Built for
Cortex-M4 at `-Os` the engine is 5.0 KB of code and 25 KB of RAM including the table
(`RV_TT_BITS=10`), measured by the `cross` job on every push.

### Verification

`make test` (unit tests, perft, reference search), `make smoke` (CLI), `make asan`, `make bench`,
`make cross` (Cortex-M4 build). CI runs gcc and clang with and without ASan/UBSan, valgrind,
cppcheck and scan-build, and the cross build.

### Where it can go

MCU port (the Cortex-M4 build already passes in CI), WASM build in a separate repo, a stdio match
protocol for playing other engines, and learning evaluation weights from self-play. See
`docs/spec.md`.

### License

MIT
