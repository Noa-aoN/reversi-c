CC      ?= cc
BUILD   ?= build
CFLAGS  ?= -O2
WARN     = -std=c11 -Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion -Wstrict-prototypes
CPPFLAGS = -Iinclude
SAN      = -fsanitize=address,undefined -fno-omit-frame-pointer

LIB_SRC  = $(wildcard src/*.c)
LIB_OBJ  = $(LIB_SRC:src/%.c=$(BUILD)/%.o)
TEST_SRC = $(wildcard tests/*.c)
TEST_BIN = $(TEST_SRC:tests/%.c=$(BUILD)/%)

.PHONY: all test asan valgrind bench clean

all: $(BUILD)/reversi

$(BUILD)/reversi: src/cli/main.c $(LIB_OBJ)
	@mkdir -p $(@D)
	$(CC) $(WARN) $(CFLAGS) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD)/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(WARN) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(BUILD)/test_%: tests/test_%.c $(LIB_OBJ)
	@mkdir -p $(@D)
	$(CC) $(WARN) $(CFLAGS) $(CPPFLAGS) -Itests -o $@ $^ $(LDFLAGS)

$(BUILD)/bench_%: bench/bench_%.c $(LIB_OBJ)
	@mkdir -p $(@D)
	$(CC) $(WARN) $(CFLAGS) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

test: $(TEST_BIN)
	@for t in $(TEST_BIN); do ./$$t || exit 1; done

asan:
	$(MAKE) test BUILD=$(BUILD)/asan CFLAGS="-O1 -g $(SAN)" LDFLAGS="$(SAN)"

valgrind: $(TEST_BIN)
	@for t in $(TEST_BIN); do \
		valgrind -q --error-exitcode=1 --leak-check=full ./$$t || exit 1; \
	done

bench: $(BUILD)/bench_perft $(BUILD)/bench_search
	./$(BUILD)/bench_perft
	./$(BUILD)/bench_search

clean:
	rm -rf $(BUILD)
