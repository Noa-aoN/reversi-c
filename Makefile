CC      ?= cc
BUILD   ?= build
CFLAGS  ?= -O2
WARN     = -std=c11 -Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion -Wstrict-prototypes
CPPFLAGS = -Iinclude
SAN      = -fsanitize=address,undefined -fno-omit-frame-pointer
CROSS   ?= arm-none-eabi-
MCU     ?= -mcpu=cortex-m4 -mthumb

LIB_SRC  = $(wildcard src/*.c)
LIB_OBJ  = $(LIB_SRC:src/%.c=$(BUILD)/%.o)
TEST_SRC = $(wildcard tests/*.c)
TEST_BIN = $(TEST_SRC:tests/%.c=$(BUILD)/%)
CROSS_OBJ = $(LIB_SRC:src/%.c=$(BUILD)/cross/%.o)

.PHONY: all test smoke asan valgrind bench cross clean

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

smoke: $(BUILD)/reversi
	sh tests/smoke.sh $(BUILD)/reversi

asan:
	$(MAKE) test BUILD=$(BUILD)/asan CFLAGS="-O1 -g $(SAN)" LDFLAGS="$(SAN)"

valgrind: $(TEST_BIN)
	@for t in $(TEST_BIN); do \
		valgrind -q --error-exitcode=1 --leak-check=full ./$$t || exit 1; \
	done

bench: $(BUILD)/bench_perft $(BUILD)/bench_search
	./$(BUILD)/bench_perft
	./$(BUILD)/bench_search

$(BUILD)/cross/%.o: src/%.c
	@mkdir -p $(@D)
	$(CROSS)gcc $(WARN) $(MCU) -Os $(CPPFLAGS) -DRV_TT_BITS=10 -c -o $@ $<

cross: $(CROSS_OBJ)
	$(CROSS)size -t $^

clean:
	rm -rf $(BUILD)
