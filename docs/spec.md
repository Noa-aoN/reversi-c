# reversi-c spec

設計判断とその理由。各節とも新しい項目を下に足す。

[English](#english)

## 盤面表現

- 手番側から見た `own` / `opp` の 2 本の `uint64_t` と、絶対色を復元するための `side`。
  着手時に 2 本を入れ替えるので、探索コードは色で分岐しない。
- マス番号は `rank * 8 + file`、`a1 = 0`、`h8 = 63`。表示上の上から下が rank 1 → 8 で、
  一般的なオセロ棋譜 (`f5` が定番の初手) と一致する。
- 方向シフトは、折り返す file をマスクで落とす (東向きなら `~FILE_A`、西向きなら `~FILE_H`)。
  南北は語長から自然にこぼれる。

## 合法手生成と反転

- 合法手: 8 方向それぞれで `own` を `opp` 上に最大 6 回流し、最後に 1 回シフトして空マスに乗せる。
  マスごとのループを持たない。
- 反転: 着手ビットから同じ流し方をし、最後のシフトが `own` に当たった方向だけ採用する。
- マスごとのループなしで正しい最も単純な形。Kogge-Stone 型の並列プレフィックス版の方が速いので、
  ベンチ段階で置き換え候補。

## perft の意味論

- `perft(0) = 1`。パスは 1 手として数える。両者とも打てない局面は残り深さに関係なく 1 葉。
- この規則で参照列 4, 12, 56, 244, 1396, 8200, 55092, 390216, 3005288, 24571284 (1〜10 手) と一致。
  最短ゲームが 9 手なので、10 手目で初めてパス規則と終局規則の両方が効く。
  ここが一致することで両方の選択が固定される。

## プレイヤー (段階 2)

- `random`: xorshift64* を自前で持つ。`rand()` を使わないのは、シードを与えれば
  どの libc でも同じ棋譜になるようにするため (再現テストと将来の自己対戦で必要)。
- `greedy`: 反転数最大の手。同点はマス番号が小さい方 (決定的)。探索の基準線として残す。
- どちらも合法手がなければ `RV_PASS` を返し、パスの判断は呼び出し側 (CLI / 探索) が行う。

## CLI

- 盤面は `X` 黒 / `O` 白 / `.` 合法手 / `-` 空き。手番がなければ自動パスして表示する。
- 棋譜は `f5d6c3...` の連結文字列。パスは記録しない (`replay` が必要に応じて自動パスを挟むため)。
- `MAX_PLIES = 128` の固定長配列に履歴を持つ。実際の上限は 60 手 + パスなので十分。

## 評価関数 (段階 3)

- 手番側から見たスコア。マスを重み別の bitmask (角 100 / X -50 / C -20 / A 10 / B 5 / 内周 -2 / 中央 -1)
  に分け、`popcount(own & mask) - popcount(opp & mask)` の加重和で求める。64 マスのループを持たない。
- 角が埋まったら、その角に隣接する X / C マスのペナルティを打ち消す (角が取られた後の X は危険ではない)。
- 着手可能数の差 × 8。終局は石差 × 1000 (`RV_SCORE_DISC`) で、静的評価を必ず支配する。
- 反転対称 (`eval(b) == -eval(swap(b))`) をランダム局面でテストしている。

## 探索 (段階 3)

- negamax + αβ。パスは深さを消費しない (両者パスは終局として石差を返す)。
- 手順前後: 角 → 普通のマス → C → X の 4 バケツ順に探索する。反復深化では前回の最善手を最初に読む。
- 深さは `empties` で頭打ちにするので、終盤は自然に完全読みになる (石差で確定したスコア)。
- 正しさの根拠: 枝刈りなしの参照 negamax (テスト内に実装) とランダム局面 40 件で深さ 3 のスコアが一致、
  残り 8 マス以下の局面 15 件で完全読みのスコアが一致。
- 置換表・時間制御は段階 4。

## メモリと移植性

- `src/*.c` でヒープ確保をしない。`rv_board` は探索中に値コピーする。
- コンパイラ拡張は `#if defined(__GNUC__)` の内側のみ (`popcount` / `ctz` は移植可能な代替あり)。
- `printf` 系は盤面整形と CLI だけ。

## テスト

- `tests/minitest.h` はカウンタと 2 つのマクロのみ。動的登録も setjmp もない。
- `tests/test_*.c` はそれぞれ独立した実行ファイル。`make test` が全部回し、最初の失敗で止まる。
- 構造体の等値比較は `memcmp` ではなくフィールドで行う。`rv_board` には enum の後ろに
  パディングがあり、valgrind が未初期化読みとして検出した。

---

## English

Design decisions and the reasoning behind them. Newest entries go at the bottom of each section.

### Board representation

- Two `uint64_t` bitboards, `own` and `opp`, always from the side-to-move's point of view, plus
  a `side` tag so the caller can recover absolute colors. Making a move swaps the two words.
  Search code therefore never branches on color.
- Square index is `rank * 8 + file`, `a1 = 0`, `h8 = 63`. Ranks run top to bottom as printed,
  which matches standard Othello notation (`f5` is the usual first move).
- Direction shifts mask off the wrapped file (`~FILE_A` after an east-going shift, `~FILE_H`
  after a west-going shift). North/south shifts fall off the end of the word naturally.

### Move generation and flipping

- Legal moves: for each of 8 directions, flood `own` through `opp` up to 6 times, then one more
  shift onto an empty square. 8 x 7 shifts, no per-square loop.
- Flips for a chosen square: same flood starting from the move bit; the run only counts if the
  final shift lands on an `own` stone.
- This is the simplest formulation that is correct and has no per-square loop. Kogge-Stone style
  parallel-prefix versions are faster and may replace it in the benchmark stage.

### Perft semantics

- `perft(0) = 1`.
- If the side to move has no legal move but the opponent does, the pass consumes one ply.
- If neither side can move, the position is one leaf regardless of remaining depth.
- With these rules the counts match the reference sequence
  4, 12, 56, 244, 1396, 8200, 55092, 390216, 3005288, 24571284 (depth 1 to 10).
  Depth 10 is the first depth where both the pass rule and the terminal rule matter
  (the shortest possible game is 9 plies), so matching it pins both choices.

### Players (stage 2)

- `random` uses an in-tree xorshift64* rather than `rand()`, so a given seed produces the same
  game on every libc (needed for reproducible tests and later self-play).
- `greedy` picks the move with the most flips, lowest square on ties. It stays as the baseline
  for search.
- Both return `RV_PASS` when there is no legal move; the caller decides how to pass.

### CLI

- Board shows `X` black, `O` white, `.` legal move, `-` empty. A side with no move passes
  automatically and says so.
- Transcript is the concatenated string `f5d6c3...`. Passes are not recorded; `replay` inserts
  them when needed.
- History lives in a fixed `MAX_PLIES = 128` array; the real bound is 60 moves plus passes.

### Evaluation (stage 3)

- Score from the side to move. Squares are grouped into weight-class bitmasks (corner 100 / X -50 /
  C -20 / A 10 / B 5 / inner ring -2 / center -1) and summed as
  `popcount(own & mask) - popcount(opp & mask)`. No 64-square loop.
- Once a corner is occupied, the X and C penalties next to it are cancelled.
- Mobility difference x 8. Finished games score disc difference x 1000 (`RV_SCORE_DISC`), which
  always dominates the static terms.
- Antisymmetry (`eval(b) == -eval(swap(b))`) is tested on random positions.

### Search (stage 3)

- Negamax with alpha-beta. A pass does not consume depth; two passes are a terminal node.
- Move ordering in four buckets: corners, ordinary squares, C squares, X squares. Iterative
  deepening searches the previous best move first at the root.
- Depth is capped at the number of empties, so the endgame becomes an exact solve with a
  disc-difference score.
- Correctness: a pruning-free reference negamax (in the test file) agrees on 40 random positions at
  depth 3 and on 15 exact solves with 8 or fewer empties.
- Transposition table and time control are stage 4.

### Memory and portability rules

- No heap allocation anywhere in `src/*.c`. `rv_board` is copied by value during search.
- No compiler extensions outside `#if defined(__GNUC__)` guards (`popcount`, `ctz` have
  portable fallbacks).
- `printf`-family only in the board formatter and the CLI.

### Testing

- `tests/minitest.h`: counters plus two macros. No dynamic registration, no setjmp, so it also
  works where a full framework would not.
- Each `tests/test_*.c` is its own executable; `make test` runs them all and stops at the first
  failure.
- Struct equality is checked field by field, never with `memcmp`: `rv_board` has padding after
  the enum and valgrind flagged the uninitialised read.
