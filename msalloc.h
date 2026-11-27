/*
 * Author: strommach
 * Date: 26th November 2026
 * msalloc.h - v0.0.1: Lightweight single-header C99 custom memory allocators.
 *
 * Usage:
 *   Just #include "msalloc.h" in any C99 (or C++) translation unit.
 *
 * Allocators provided:
 *   1. Arena / Bump Allocator (ArenaAllocator):
 *        Fast linear allocation with power-of-two alignment. Freed all at once
 *        via arena_clear() or arena_destroy().
 *        API: arena_init, arena_new, arena_alloc, arena_clear, arena_destroy
 *
 *   2. Stack Allocator (StackAllocator):
 *        Linear allocator supporting LIFO (last-in, first-out) deallocation.
 *        API: stackalloc_init, stackalloc_new, stackalloc_alloc,
 *             stackalloc_free, stackalloc_clear, stackalloc_destroy
 *
 *   3. Slab Allocator (SlabAllocator):
 *        Placeholder struct (unimplemented).
 *
 * Notes:
 *   - Passing memblock == NULL to *_init() (or calling *_new()) heap-allocates
 *     the backing buffer; *_destroy() will free it.
 *   - Passing a non-NULL memblock uses caller-owned memory; *_destroy() will
 *     NOT free it.
 *   - All alignment arguments must be non-zero powers of two.
 */

#ifndef MSALLOC_H
#define MSALLOC_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef MSA_INLINE
#if defined(__cplusplus)
#define MSA_INLINE inline
#elif defined(__GNUC__) || defined(__clang__)
#define MSA_INLINE static inline __attribute__((unused))
#else
#define MSA_INLINE static inline
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ==========================================================================
 * Generic Allocator VTable
 * ========================================================================== */

typedef struct {
  void *(*alloc)(void *ctx, size_t size);
  void (*free)(void *ctx, void *ptr);
} AllocatorVTable;

/* ==========================================================================
 * Internal helpers
 * ========================================================================== */

/* Rounds addr up to the nearest multiple of align (power of two). */
MSA_INLINE uintptr_t __msa_align_up(uintptr_t addr, size_t align) {
  assert(align > 0 && (align & (align - 1)) == 0 &&
         "Alignment must be a power of 2");
  return (addr + (align - 1U)) & ~(uintptr_t)(align - 1U);
}

/* ==========================================================================
 * Arena / Bump Allocator
 * ========================================================================== */

typedef struct {
  uint8_t *buffer;
  size_t capacity;
  size_t offset;
  uint8_t owns_buf;
} ArenaAllocator;

/*
 * Initializes an arena with capacity cap bytes.
 * If memblock is NULL, allocates a buffer on the heap; otherwise uses memblock
 * directly (zeroed) without taking ownership.
 */
MSA_INLINE void arena_init(ArenaAllocator *arena, size_t cap,
                           uint8_t *memblock) {
  assert(arena != NULL);
  arena->offset = 0;

  if (!memblock) {
    arena->buffer = cap > 0 ? (uint8_t *)malloc(cap) : NULL;
    arena->capacity = arena->buffer ? cap : 0;
    arena->owns_buf = (arena->buffer != NULL);
  } else {
    memset(memblock, 0, cap);
    arena->buffer = memblock;
    arena->capacity = cap;
    arena->owns_buf = 0;
  }
}

/* Creates a new heap-backed arena of cap bytes. */
MSA_INLINE ArenaAllocator arena_new(size_t cap) {
  ArenaAllocator arena;
  arena_init(&arena, cap, NULL);
  return arena;
}

/*
 * Allocates size zero-initialized bytes aligned to align (power of two).
 * Returns NULL if the arena has insufficient remaining capacity.
 */
MSA_INLINE void *arena_alloc(ArenaAllocator *arena, size_t size, size_t align) {
  assert(arena != NULL);
  if (!arena->buffer || size == 0) {
    return NULL;
  }

  uintptr_t curr_addr = (uintptr_t)arena->buffer + (uintptr_t)arena->offset;
  uintptr_t aligned_addr = __msa_align_up(curr_addr, align);
  size_t start = (size_t)(aligned_addr - (uintptr_t)arena->buffer);
  if (start > arena->capacity || size > arena->capacity - start) {
    return NULL;
  }

  arena->offset = start + size;
  return memset((void *)aligned_addr, 0, size);
}

/* Resets the arena offset to 0, invalidating all prior allocations. */
MSA_INLINE void arena_clear(ArenaAllocator *arena) {
  assert(arena != NULL);
  arena->offset = 0;
}

/* Releases the backing buffer if owned by the arena and resets all fields. */
MSA_INLINE void arena_destroy(ArenaAllocator *arena) {
  assert(arena != NULL);
  if (arena->owns_buf && arena->buffer) {
    free(arena->buffer);
  }
  arena->buffer = NULL;
  arena->capacity = 0;
  arena->offset = 0;
  arena->owns_buf = 0;
}

/* ==========================================================================
 * Stack Allocator (LIFO)
 * ========================================================================== */

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

/*
 * Initializes a stack allocator with capacity cap bytes.
 * If memblock is NULL, allocates a buffer on the heap; otherwise uses memblock
 * without taking ownership.
 */
MSA_INLINE void stackalloc_init(StackAllocator *stack, size_t cap,
                                uint8_t *memblock) {
  assert(stack != NULL);
  stack->offset = 0;
  stack->last = NULL;

  if (!memblock) {
    uint8_t *buffer = cap > 0 ? (uint8_t *)malloc(cap) : NULL;
    stack->buffer = buffer;
    stack->owns_buf = (buffer != NULL);
    stack->capacity = buffer ? cap : 0;
  } else {
    stack->buffer = memblock;
    stack->owns_buf = 0;
    stack->capacity = cap;
  }
}

/* Creates a new heap-backed stack allocator of cap bytes. */
MSA_INLINE StackAllocator stackalloc_new(size_t cap) {
  StackAllocator stack;
  stackalloc_init(&stack, cap, NULL);
  return stack;
}

/*
 * Allocates size zero-initialized bytes aligned to align (power of two).
 * Allocations must be freed in strict LIFO order via stackalloc_free().
 * Returns NULL if insufficient space remains.
 */
MSA_INLINE void *stackalloc_alloc(StackAllocator *stack, size_t size,
                                  size_t align) {
  assert(stack != NULL);
  if (!stack->buffer || size == 0) {
    return NULL;
  }

  /* Ensure payload alignment is at least StackAllocHeader's alignment */
  struct __msa_hdr_align {
    char c;
    StackAllocHeader h;
  };
  size_t hdr_align = offsetof(struct __msa_hdr_align, h);
  if (align < hdr_align) {
    align = hdr_align;
  }

  uintptr_t base = (uintptr_t)stack->buffer;
  uintptr_t curr_addr = base + (uintptr_t)stack->offset;
  uintptr_t min_payload_addr = curr_addr + sizeof(StackAllocHeader);
  uintptr_t aligned_payload_addr = __msa_align_up(min_payload_addr, align);
  size_t payload_offset = (size_t)(aligned_payload_addr - base);

  if (payload_offset > stack->capacity ||
      size > stack->capacity - payload_offset) {
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

/*
 * Frees the most recent allocation from the stack allocator.
 * ptr must match the last allocation (LIFO order); passing NULL is a no-op.
 */
MSA_INLINE void stackalloc_free(StackAllocator *stack, void *ptr) {
  assert(stack != NULL);
  if (!ptr) {
    return;
  }

  assert(stack->buffer != NULL && "Stack allocator is not initialized");

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

/* Resets the stack allocator, invalidating all active allocations. */
MSA_INLINE void stackalloc_clear(StackAllocator *stack) {
  assert(stack != NULL);
  stack->offset = 0;
  stack->last = NULL;
}

/* Releases the backing buffer if owned by the stack allocator. */
MSA_INLINE void stackalloc_destroy(StackAllocator *stack) {
  assert(stack != NULL);
  if (stack->owns_buf && stack->buffer) {
    free(stack->buffer);
  }
  stack->buffer = NULL;
  stack->capacity = 0;
  stack->offset = 0;
  stack->last = NULL;
  stack->owns_buf = 0;
}

/* ==========================================================================
 * Slab Allocator (unimplemented)
 * ========================================================================== */

typedef struct {
  uint8_t *buffer;
  size_t capacity;
} SlabAllocator;

#ifdef __cplusplus
}
#endif

#endif /* MSALLOC_H */
