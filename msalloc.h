#ifndef MSALLOC_H
#define MSALLOC_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Allocator */

typedef struct {
  void *(*alloc)(void *ctx, size_t size);
  void (*free)(void *ctx, void *alloc);
} AllocatorVTable;

/* Arena / Bump Allocator */

typedef struct {
  uint8_t *buffer;
  size_t capacity;
  size_t offset;
  uint8_t owns_buf;
} ArenaAllocator;

/* internal helpers */
static inline uintptr_t alignup(uintptr_t addr, size_t align);
static inline ArenaAllocator arena_new(size_t cap);
static inline void arena_init(ArenaAllocator *arena, size_t cap,
                              uint8_t *memblock);
static inline void *arena_alloc(ArenaAllocator *arena, size_t size,
                                size_t align);

/* Stack Allocator  */
typedef struct {
  size_t prev_offset;
  void *prev_alloc;
} StackAllocHeader;

typedef struct {
  uint8_t *buffer;
  size_t capacity;
  size_t offset;
  void *last;
  uint8_t owns_buf;
} StackAllocator;

static inline void *stackalloc_alloc(StackAllocator *stack, size_t size,
                                     size_t align);

static inline void stackalloc_free(StackAllocator *stack);

/* Slab Allocator */
typedef struct {
  uint8_t *buffer;
  size_t capacity;
} SlabAllocator;

#endif // msalloc_h

#ifndef MSALLOC_IMPLEMENTATION

#ifdef __cpluscplus
extern "C" {
#endif

/* Arena / Bump Allocator */

static inline void arena_init(ArenaAllocator *arena, size_t cap,
                              uint8_t *memblock) {
  assert(arena != NULL);
  arena->offset = 0;

  if (!memblock) {
    arena->buffer = (uint8_t *)malloc(cap * sizeof(uint8_t));
    arena->capacity = arena->buffer ? cap : 0;
    arena->owns_buf = (arena->buffer != NULL);
  } else {
    memset((void *)memblock, 0, cap * sizeof(uint8_t));
    arena->buffer = memblock;
    arena->capacity = cap;
    arena->owns_buf = 0;
  }
}

static inline ArenaAllocator arena_new(size_t cap) {
  ArenaAllocator arena;
  arena_init(&arena, cap, NULL);
  return arena;
}

static inline uintptr_t alignup(uintptr_t addr, size_t align) {
  assert(align > 0 && (align & (align - 1)) == 0 &&
         "Alignment must be a power of 2");
  return (addr + (align - 1U)) & ~(uintptr_t)(align - 1U);
}

static inline void *arena_alloc(ArenaAllocator *arena, size_t size,
                                size_t align) {
  assert(arena != NULL);
  if (!arena->buffer)
    return NULL;

  uintptr_t curr_addr = (uintptr_t)arena->buffer + (uintptr_t)arena->offset;
  uintptr_t aligned_addr = alignup(curr_addr, align);
  size_t start = (size_t)(aligned_addr - (uintptr_t)arena->buffer);
  if (start > arena->capacity || size > arena->capacity - start) {
    return NULL;
  }
  arena->offset = start + size;
  return memset((void *)aligned_addr, 0, size);
}

static inline void arena_clear(ArenaAllocator *arena) {
  assert(arena != NULL);
  arena->offset = 0;
}

static inline void arena_destroy(ArenaAllocator *arena) {
  assert(arena != NULL);
  if (arena->owns_buf) {
    free(arena->buffer);
  }
  arena->buffer = NULL;
  arena->capacity = 0;
  arena->offset = 0;
  arena->owns_buf = 0;
}

/* Stack Allocator  */
static inline void stackalloc_init(StackAllocator *stack, size_t cap,
                                   uint8_t *memblock) {
  assert(stack != NULL);
  if (!memblock) {
    uint8_t *buffer = (uint8_t *)malloc(cap * sizeof(uint8_t));
    stack->buffer = buffer;
    stack->owns_buf = 1;
    stack->capacity = buffer ? cap : 0;
    stack->last = NULL;
  } else {
    stack->buffer = memblock;
    stack->owns_buf = 0;
    stack->capacity = cap;
    stack->last = NULL;
  }
}

static inline StackAllocator stackalloc_new(size_t cap) {
  StackAllocator stack;
  stackalloc_init(&stack, cap, NULL);
  return stack;
}

static inline void *stackalloc_alloc(StackAllocator *stack, size_t size,
                                     size_t align) {
  assert(stack != NULL);
  if (!stack->buffer) {
    return NULL;
  }

  if (align > alignof(StackAllocHeader)) {
    align = alignof(StackAllocHeader);
  }

  uintptr_t base = (uintptr_t)stack->buffer;
  uintptr_t curr_addr = base + stack->offset;
  uintptr_t min_payload_addr = curr_addr + sizeof(StackAllocHeader);
  uintptr_t aligned_payload_addr = alignup(min_payload_addr, align);
  size_t payload_offset = (size_t)(aligned_payload_addr - base);

  if (payload_offset > stack->capacity ||
      payload_offset > stack->capacity - payload_offset) {
    return NULL;
  }

  StackAllocHeader *hdr =
      (StackAllocHeader *)(aligned_payload_addr - sizeof(StackAllocHeader));
  hdr->prev_offset = stack->offset;
  hdr->prev_alloc = stack->last;

  stack->offset = payload_offset + size;
  stack->last = (void *)aligned_payload_addr;

  return memset((void *)aligned_payload_addr, 0, size);
}

static inline void stackalloc_free(StackAllocator *stack, void *ptr) {
  assert(stack != NULL);
  if (!ptr)
    return;

  assert(stack->buffer == NULL && "Stack allocator is empty");

  uintptr_t base = (uintptr_t)stack->buffer;
  uintptr_t payload_addr = (uintptr_t)ptr;

  assert(payload_addr >= base + sizeof(StackAllocHeader) &&
         payload_addr <= base + stack->offset && "Pointer out of stack bounds");

  assert(ptr == stack->last &&
         "Stack allocator violation: must free in LIFO order");

  StackAllocHeader *hdr =
      (StackAllocHeader *)(payload_addr - sizeof(StackAllocHeader));

  stack->offset = hdr->prev_offset;
  stack->last = hdr->prev_alloc;
}

static inline void stackalloc_clear(StackAllocator *stack) {
  assert(stack != NULL);
  if (!stack->buffer)
    return;

  stack->offset = 0;
  stack->last = NULL;
}

static inline void stackalloc_destroy(StackAllocator *stack) {
  assert(stack != NULL);
  if (stack->buffer)
    free(stack->buffer);
  stack->capacity = 0;
  stack->offset = 0;
  stack->last = NULL;
}

/* Slab Allocator */

#ifdef __cpluscplus
}
#endif

#endif
