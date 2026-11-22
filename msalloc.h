#ifndef msalloc_h
#define msalloc_h

#include <stddef.h>
#include <stdlib.h>

typedef struct {
  void *(*alloc)(void *ctx, size_t size);
  void (*free)(void *ctx, void *alloc);
} AllocatorVTable;

#endif // msalloc_h

#ifndef MSALLOC_IMPLEMENTATION

#ifdef __cpluscplus
extern "C" {
#endif

// Arena / Bump Allocator
typedef struct {
  size_t cap;
  size_t len;
  void *bytes;
  AllocatorVTable vtable;
} ArenaAllocator;

void *arena_alloc(size_t size) {}

void arena_free(ArenaAllocator *alloc) {}

ArenaAllocator create_arena(size_t capacity) {
  return (ArenaAllocator){
      .len = 0,
      .cap = capacity,
      .bytes = malloc(capacity),
      .vtable = (AllocatorVTable){.alloc = arena_alloc, .free = arena_free}};
}

// Stack Allocator

// Slab Allocator

#ifdef __cpluscplus
};
#endif

#endif
