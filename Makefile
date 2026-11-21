CC      ?= cc
STD     ?= c99
CFLAGS  ?= -std=$(STD) -O2 -Wall -Wextra -Wpedantic -Werror
ASAN_FLAGS ?= -std=$(STD) -g -O1 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer
BUILD_DIR ?= build

.PHONY: all test hot build clean
