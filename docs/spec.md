# reversi-c spec

設計判断とその理由、性能の記録、今後の活用の考察。README が「流れ」なら、こちらは「なぜそうしたか」です。

[English](#english)

## 目的と制約

このリポジトリの目的は、組み込みで書いてきた C を公開の形にすることです。だから UI より中身、
見た目より検証に寄せています。最初に決めて最後まで守った制約は次の 3 つです。

1. 外部ライブラリを使わない。テストフレームワークも自作のヘッダ 1 つ。
2. エンジンはヒープを使わない。固定長の配列と値コピーだけで動く。
3. 正しさは機械で確かめる。perft の参照値、枝刈りなしの参照探索、ASan / UBSan / valgrind。

制約 2 は性能のためというより、移植性と見積もりのためです。ヒープがなければ RAM 使用量は
リンク時に決まり、マイコンに持っていくときにそのまま読めます。

## 盤面表現

- `own` / `opp` の 2 本の `uint64_t`。常に手番側から見た形にし、絶対色は `side` で持ちます。
  着手は「反転して入れ替える」だけで、探索コードに色の分岐がありません。
- マス番号は `rank * 8 + file`、`a1 = 0`、`h8 = 63`。画面の上が rank 1 です。
  一般的な棋譜表記 (`f5` が定番の初手) と一致します。
- 局面は 24 バイトなので、探索中は undo を持たず値コピーで子局面を作ります。
  undo の実装ミスという定番のバグをそもそも作らない選択です。

## 合法手生成 (Kogge-Stone)

- 「相手石が続き、その先に自分の石」という条件を 8 方向分、bitboard 全体に対して一度に求めます。
- 相手石の上を流す処理は occluded fill と呼ばれる並列プレフィックスで、1, 2, 4 マスのシフト 3 回で
  最長 6 個の相手石をたどれます。初版は 1 マスずつ 6 回流していたので、シフト回数は半分になりました。
- 盤の端をまたがないよう、n マスのシフトでは n 列ぶんをマスクで落とします (東向きなら a 列側、
  西向きなら h 列側)。南北のシフトは語長からこぼれるので何もしません。
- 置き換えの正しさは perft で確認しました。1〜10 手の値が置き換えの前後で同一です。

## perft の定義

- `perft(0) = 1`。
- 手番側に合法手がなく相手にはある場合、パスを 1 手と数える。
- 両者とも打てない局面は、残り深さに関係なく 1 局面と数える。
- この定義で参照列 4, 12, 56, 244, 1396, 8200, 55092, 390216, 3005288, 24571284 (1〜10 手) と
  一致します。最短の終局が 9 手なので、10 手目ではじめてパスと終局の扱いが数に現れます。
  10 手目まで一致したことで、定義そのものが固定されています。
- `rv_perft_divide` (CLI では `divide <depth>`) は初手ごとの内訳を出します。数が合わないとき、
  他エンジンの divide と突き合わせてどの手の先で差が出るかを絞り込むための定番の道具です。

## 探索

- negamax + αβ。パスは深さを消費しません。(局面, 深さ) の値がそこまでの経路に依存しないように
  するためで、置換表と参照探索の一致にも必要な条件です。両者パスは終局として石差を返します。
- 手順前後は角 → 普通のマス → C → X の 4 バケツ。反復深化では前回の最善手を最初に読みます。
- 深さは空きマス数で頭打ちにしています。終盤の完全読みに別のコードはなく、同じ探索が
  自然に読み切ります。読み切った結果は `rv_search_result.exact` で分かります。
- 正しさは、枝刈りなしの参照 negamax (テスト内に実装) と、ランダム局面 40 件 (深さ 3)、
  残り 8 マス以下の局面 15 件 (読み切り) でスコアが一致することで確認しています。

## 評価関数

- 手番側から見たスコア。マスを種類ごとに bitmask にまとめ (角 100 / X -50 / C -20 / A 10 /
  B 5 / 内周 -2 / 中央 -1)、`popcount(own & mask) - popcount(opp & mask)` の加重和を取ります。
  64 マスのループは持ちません。
- 角が埋まったら、その隣の X・C マスの罰則を打ち消します。角を取られた後の X は危険ではないためです。
- 着手可能数の差 x 8。終局は石差 x 1000 (`RV_SCORE_DISC`) で、静的評価を必ず上回ります。
- 反転対称 (`eval(b) == -eval(swap(b))`) をランダム局面で確認しています。
- 重みは手で置いたもので、学習はしていません。それでも「深く読むほど勝つ」順序は保たれています。

## 置換表

- ハッシュは Zobrist ではなく、`own` と `opp` を splitmix64 の finalizer で混ぜたものです。
  表が要らず O(1) で、局面構造体から直接計算できます。手番側視点の bitboard がそのまま鍵なので、
  同じ形は手番に関係なく同じエントリに当たります (Reversi のルールは色対称)。
- エントリには `own` / `opp` を丸ごと入れ、完全一致でだけ hit とします。偽ヒットは原理的に起きません。
  代わりに 1 エントリ 24 バイトで、2^18 エントリで 6 MiB の静的配列です (`RV_TT_BITS`)。
- 置き換えは「同じ局面でより浅い結果は上書きしない」以外は常に上書きです。
- 値に使うのは同じ深さのエントリだけです。深いエントリは手順前後にだけ使います。
  深い結果を値に使う方が強いのですが、そうすると固定深さの評価値が表の中身で変わり、
  参照探索との一致テストが成り立たなくなります。検証できることを優先しました。切り替えは 1 行です。

## 時間制御

- 反復深化 + 4096 ノードごとの時計確認。締切を過ぎたら探索を中断し、直前に完了した深さの結果を使います。
  中断した反復の結果は置換表にも書きません。
- 時計はエンジンが持たず、`rv_search_set_clock` でホストから秒を返す関数を受け取ります。既定は C89 の
  `clock()` で、CLI は `timespec_get` の壁時計を渡します。最初は `timespec_get` を直接呼んでいましたが、
  newlib (arm-none-eabi) にはなく Cortex-M4 ビルドが通らなかったので、この形にしました。
  テストでは偽の時計を渡して、締切超過で直前の深さの結果が返ることを決定的に確かめています。

## テスト方針

- `tests/minitest.h` はカウンタと 2 つのマクロだけ。動的登録も setjmp もありません。
- `tests/test_*.c` はそれぞれ独立した実行ファイルで、`make test` が全部回して最初の失敗で止まります。
- CLI は `tests/smoke.sh` で各サブコマンドを 1 回ずつ通します (`make smoke`)。エンジンの正しさは
  単体テストで見るので、ここは引数の解釈と出力の形だけを見ます。
- `make cross` は arm-none-eabi-gcc でエンジンだけを Cortex-M4 向けにコンパイルし、`size` を出します。
  CI でも回していて、移植性の主張をビルドで裏付けています。
- 静的解析は cppcheck (`warning,performance,portability`) と clang の scan-build を CI でかけます。
  手元の Apple clang でも `cc --analyze` で同じ解析が走ります。
- 構造体の等値比較は `memcmp` ではなくフィールドで行います。`rv_board` は enum の後ろに
  パディングがあり、CI の valgrind が未初期化読みとして検出しました。
- 合法手生成と反転計算は、盤の上下反転・左右反転・180 度回転と可換であることをランダム局面で確認します。
  perft は数しか見ないので、方向ごとのシフトとマスクの非対称なバグを別の角度から押さえるためです。
- 乱数は `rand()` ではなく自前の xorshift64* です。シードを固定すればどの libc でも同じ棋譜になり、
  自己対戦のテストが決定的になります。

## 性能の記録

Apple M 系 1 コア、`-O2`。

| 変更 | perft(10) | ab8 vs ab6 20 局 |
|---|---:|---:|
| 初版 (6 回シフト、置換表なし) | 0.91 s (26 M/s) | 19.1 s |
| 置換表 | 同じ | 4.4 s |
| Kogge-Stone 充填 | 0.51 s (48 M/s) | 3.1 s |

探索は 8 手読み x 20 局面で 12 M ノード/秒 (`make bench`)。エンジンのコードサイズは Cortex-M4 向け `-Os` で 5.0 KB
(置換表を除く)。

## 今後の活用

エンジンは UI にも OS にも依存していないので、そのまま別の場所へ持っていけます。
やるならこの順が自然だと考えています。

### 1. マイコンへの移植

- 必要なのは `src/*.c` のうち CLI 以外と `include/`。`printf` を使うのは盤面整形だけです。
- RAM の見積もり: 局面 24 バイト、探索の再帰は最大 60 段で数 KB、置換表は `RV_TT_BITS=10` で 24 KB。
  Cortex-M4 (RAM 64 KB 程度) で十分収まります。置換表を外せば 10 KB 未満です。
- コードは Cortex-M4 向け `-Os` で 5.0 KB (text)、RAM は `RV_TT_BITS=10` で 25 KB (bss)。CI の
  `cross` ジョブが毎回この構成でビルドして `size` を出します。`popcount` / `ctz` は GCC 拡張を
  使っていますが、`#if` の外側に移植可能な実装があるので、どのコンパイラでもビルドできます。
- 時計は `rv_search_set_clock` に SysTick などを読む関数を渡します。それ以外に OS や libc に
  依存している箇所はありません (`snprintf` は盤面整形のみ)。
- 実機で動かすなら、UART 経由で盤面と手をやり取りする薄い層を足すだけです。
  「PC の CLI と同じコードがマイコンでも同じ手を返す」ことが一番分かりやすいデモになります。

### 2. ブラウザ対局 (WASM)

- Emscripten で `rv_search_timed` など数個の関数を公開し、TypeScript の薄い盤面 UI から呼びます。
- このリポジトリには入れず別リポにします。UI コードが混ざるとリポジトリの言語構成が崩れ、
  依存ゼロの前提も変わるためです。

### 3. 対戦プロトコル

- 標準入出力で局面と手をやり取りするプロトコルを足すと、他のエンジンや GUI と接続できます。
  形式は GTP (囲碁) の最小サブセットで十分です。
- 公開されている強いエンジン (Edax など) と対戦すれば、自己対戦では測れない絶対的な強さが分かります。

### 4. 評価関数の学習

- `match` で大量の自己対戦棋譜を作り、終局の石差を目的変数にして重み (角・X・C・着手可能数) を
  回帰で求めます。Logistello 以来の定番手法で、現在の手置きの重みをデータで置き換えられます。
- 学習コードは Python でも構いませんが、学習した重みを読み込む側は今の `eval.c` のまま使えます。

### 5. 教材として

- 兄弟リポジトリ `algorithms-in-c` と同じ流儀 (1 テーマ 1 ファイル、テスト併置、依存なし) で書いて
  あります。bitboard、αβ、置換表、時間制御はそれぞれ単独で読める大きさなので、
  アルゴリズムの説明の題材にそのまま使えます。

---

## English

Design decisions, performance log and future directions. The README gives the flow; this file
gives the reasons.

### Goals and constraints

The repository exists to put embedded-style C in public. Three constraints were set at the
start and kept: no third-party code (the test framework is one header), no heap in the engine
(fixed arrays and value copies only), and mechanical correctness checks (reference perft counts,
a pruning-free reference search, ASan/UBSan/valgrind). The no-heap rule is less about speed
than about portability: RAM use is known at link time.

### Board

Two `uint64_t` bitboards, `own` and `opp`, always from the side to move, plus a `side` tag.
A move flips and swaps, so the search never branches on color. Square index is
`rank * 8 + file`, `a1 = 0`. Positions are 24 bytes and are copied rather than undone, which
removes the classic undo bug by construction.

### Move generation (Kogge-Stone)

The "run of opponent stones ending in an own stone" condition is evaluated for all 64 squares at
once per direction with an occluded fill: shifts of 1, 2 and 4 cover a run of up to 6 stones,
half the shifts of the original 6-step loop. A shift of n squares masks off n files on the
wrapping side. Perft counts 1 to 10 are identical before and after the change.

### Perft

`perft(0) = 1`; a forced pass is one ply; a finished game is one leaf regardless of remaining
depth. Depths 1 to 10 match the reference sequence ending in 24,571,284. The shortest game is 9
plies, so depth 10 is the first where the pass and terminal rules matter, which pins them.
`rv_perft_divide` (CLI `divide <depth>`) splits the count by first move, the usual tool for
locating a discrepancy against another engine.

### Search

Negamax with alpha-beta. A pass does not consume depth, so the value of (position, depth) is
path-independent, which the table and the reference comparison require. Ordering: corners,
plain squares, C squares, X squares; iterative deepening seeds the previous best move. Depth is
capped at the number of empties, so the endgame is solved by the same code and flagged `exact`.
Verified against a pruning-free negamax on 40 random positions at depth 3 and 15 exact solves
with 8 or fewer empties.

### Evaluation

Weighted square-class bitmasks (corner 100 / X -50 / C -20 / A 10 / B 5 / inner -2 /
center -1) summed as popcount differences, no 64-square loop; X and C penalties lifted once the
adjacent corner is taken; mobility x 8; finished games disc difference x 1000. Antisymmetry is
tested on random positions. Weights are hand-set, not learned.

### Transposition table

Hash: splitmix64 finaliser over `own` and `opp`, no Zobrist table. Entries hold the full
position and hit only on exact match, so collisions can only miss (24 bytes per entry, 2^18
entries, `RV_TT_BITS`). A shallower result never replaces a deeper one. Only same-depth entries
supply values; deeper ones only seed ordering. Using deeper values would be stronger but would
make fixed-depth scores depend on table contents and break the reference test. Verifiability won.

### Time control

Iterative deepening with a clock check every 4096 nodes; past the deadline the search aborts
and the last completed depth is used; aborted iterations are not stored. The engine owns no
clock: `rv_search_set_clock` takes a seconds function from the host (default C89 `clock()`, the
CLI passes `timespec_get`). The first version called `timespec_get` directly, which newlib does
not have, so the Cortex-M4 build failed; a test with a fake clock now checks the abort path
deterministically.

### Testing

One-header framework, one executable per test file, field-by-field struct comparison (valgrind
caught a padding read through `memcmp`), in-tree xorshift64* so seeded self-play is
deterministic on every libc. Move generation and flips are checked to commute with the board's
vertical flip, horizontal mirror and 180-degree rotation on random positions, since perft only
compares counts. `make smoke` runs every CLI subcommand once; `make cross` compiles
the engine for Cortex-M4 with arm-none-eabi-gcc and prints `size`, in CI as well. CI also runs
cppcheck and clang's scan-build with zero findings.

### Performance log

Apple M-series core, `-O2`: perft(10) 0.91 s to 0.51 s (26 to 48 M leaves/s) with Kogge-Stone;
20-game ab8 vs ab6 match 19.1 s to 4.4 s with the table, to 3.1 s with the new generator; search
12 M nodes/s at depth 8; engine code 5.0 KB at `-Os` for Cortex-M4, 25 KB RAM with `RV_TT_BITS=10`.

### Where it can go

1. **MCU port**: engine only, no `printf` outside the formatter; 24-byte position, a few KB of
   stack, 24 KB table at `RV_TT_BITS=10`; the Cortex-M4 build passes in CI. What remains is a
   clock function and a thin UART layer. The best demo is the same code returning the same
   moves over UART as on the PC.
2. **WASM**: expose a handful of functions via Emscripten and drive them from a thin TypeScript
   board, in a separate repository to keep this one dependency-free and C-only.
3. **Match protocol**: a minimal GTP-style stdio protocol to play other engines and GUIs, which
   gives absolute strength numbers against engines such as Edax.
4. **Learned evaluation**: regress the weights on disc difference from self-play transcripts
   (the Logistello approach); `eval.c` consumes the weights unchanged.
5. **Teaching material**: each module (bitboard, alpha-beta, table, time control) reads on its
   own, in the same style as the sibling `algorithms-in-c`.
