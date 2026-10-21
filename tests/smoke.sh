#!/bin/sh
# Exercises every CLI subcommand once. Engine logic is covered by test_*.c;
# this only checks that main.c parses, prints and exits the way README says.
set -eu
bin=${1:-build/reversi}

"$bin" help | grep -q '^usage: reversi play'
"$bin" --help >/dev/null
"$bin" perft 5 | grep -q 'perft( 5) = *1396'
"$bin" divide 3 | grep -q '^f5 *14$'
"$bin" divide 3 | grep -q '^total *56$'
"$bin" show | grep -q 'X to move  X:2 O:2'
"$bin" replay f5d6c3d3c4 | grep -q 'O to move  X:6 O:3'
printf 'zz\nf5\nquit\n' | "$bin" play human human | grep -q 'moves: f5'
"$bin" play random random 1 | grep -q '^moves: '
"$bin" play greedy ab6 1 | grep -q '(exact)'
"$bin" match ab2 greedy 2 1 | grep -q 'ab2 vs greedy: 2 games'
if "$bin" play human zz 2>/dev/null; then exit 1; fi
if "$bin" 2>/dev/null; then exit 1; fi
echo "smoke: ok"
