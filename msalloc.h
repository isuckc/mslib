#ifndef msalloc_h
#define msalloc_h

#include <stddef.h>

typedef struct {
  void *ctx;
  void *(*alloc)(void *ctx, size_t size);
  void *(*free)(void *ctx, void *ptr);
} Allocator;

#endif // msalloc_h

#ifndef MSALLOC_IMPLEMENTATION

#ifdef __cpluscplus
extern "C" {
#endif

// Arena / Bump Allocator
typedef struct {
  size_t cap;
  size_t len;
  size_t offset;
  void **bytes;
} ArenaAllocator;

void *arena_alloc(size_t *capacity) {}

// Stack Allocator

// Slab Allocator

#ifdef __cpluscplus
};
#endif

#endif
