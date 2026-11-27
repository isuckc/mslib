# Mini and Simple Library (`mslib`)

A lightweight collection of portable, single-header C99 libraries with zero external dependencies. Drop any header into your project and `#include` it directly — no build system integration or implementation macros required.

## Headers

| Header | Description | Status |
| --- | --- | --- |
| [`msfs.h`](msfs.h) | Portable file I/O (whole-file helpers, streamed handles, path utilities) | Ready |
| [`msdates.h`](msdates.h) | Compact 32-bit Gregorian date packing, formatting, parsing, and arithmetic | Ready |
| [`msstrings.h`](msstrings.h) | Stretchy-buffer dynamic C strings | Ready |
| [`mshashing.h`](mshashing.h) | Non-cryptographic hashes (Wyhash, xxHash64, Murmur3, FNV-1a, DJB2) & mixers | Ready |
| [`msilist.h`](msilist.h) | Intrusive circular doubly linked list | Ready |
| [`msvector.h`](msvector.h) | Stretchy-buffer dynamic `float` vector | Ready |
| [`msalloc.h`](msalloc.h) | Custom memory allocators (Arena, LIFO Stack; Slab WIP) | Partial |
| [`mslist.h`](mslist.h) | Circular XOR linked list | WIP |
| [`mshash_map.h`](mshash_map.h) | Hash map | Planned |

## Building & Testing

Requires a C99 compiler (`cc`, `clang`, or `gcc`).

```sh
make test   # Compile and run all test suites (-std=c99 -Wall -Wextra -Wpedantic -Werror)
make asan   # Run all test suites with AddressSanitizer and UBSan
make clean  # Remove build artifacts
```
