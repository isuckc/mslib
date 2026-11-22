CC         ?= cc
STD        ?= c99
CFLAGS     ?= -std=$(STD) -O2 -Wall -Wextra -Wpedantic -Werror
ASAN_FLAGS ?= -std=$(STD) -g -O1 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer
BUILD_DIR  ?= build

.PHONY: all test asan clean

all: test

test:
	@CC="$(CC)" STD="$(STD)" CFLAGS="$(CFLAGS)" BUILD_DIR="$(BUILD_DIR)" ./tests/run_tests.sh

asan:
	@CC="$(CC)" STD="$(STD)" CFLAGS="$(ASAN_FLAGS)" BUILD_DIR="$(BUILD_DIR)" ./tests/run_tests.sh

clean:
	@rm -rf "$(BUILD_DIR)"
