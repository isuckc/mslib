/*
 * Author: strommach
 * Date: 19th November 2026
 * msvector.h - v0.0.2: Lightweight single-header C99 dynamic float vector
 * implemented with stretchy buffers.
 *
 * Usage:
 *   A vector is a regular `float *` pointer with a hidden header stored right
 *   before element 0. Functions that may reallocate (msvec_push) return the
 *   (possibly moved) pointer, so always assign the result back:
 *
 *     float *v = msvec_new(4);
 *     v = msvec_push(v, 1.0f);
 *     v = msvec_push(v, 2.0f);
 *     msvec_free(v);
 *
 * API:
 *   msvec_new     - allocate a new vector with initial capacity
 *   msvec_len     - number of elements currently stored
 *   msvec_cap     - allocated capacity (in elements)
 *   msvec_empty   - returns 1 if NULL or length == 0
 *   msvec_clear   - reset length to 0 without freeing memory
 *   msvec_push    - append a float (grows automatically, returns new pointer)
 *   msvec_pop     - remove and return the last float (or 0.0f if empty)
 *   msvec_add     - element-wise addition: result[i] = v[i] + u[i]
 *   msvec_mul     - dot product of v and u
 *   msvec_destroy - free vector memory (safe on NULL)
 *   msvec_free    - macro that frees vector and sets pointer to NULL
 */

#ifndef MSVECTOR_H
#define MSVECTOR_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef MSH_INLINE
#if defined(__cplusplus)
#define MSH_INLINE inline
#elif defined(__GNUC__) || defined(__clang__)
#define MSH_INLINE static inline __attribute__((unused))
#else
#define MSH_INLINE static inline
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  size_t cap;
  size_t len;
} vec_header;

/* Internal helpers */

MSH_INLINE vec_header *__msvec_hdr(const float *v) {
  return (vec_header *)((char *)v - sizeof(vec_header));
}

/* API */

/* Allocates a new float vector with capacity for cap elements, or NULL. */
MSH_INLINE float *msvec_new(size_t cap) {
  size_t alloc_cap = cap > 0 ? cap : 4;
  if (alloc_cap > (SIZE_MAX - sizeof(vec_header)) / sizeof(float)) {
    return NULL;
  }

  vec_header *h =
      (vec_header *)malloc(sizeof(vec_header) + sizeof(float) * alloc_cap);
  if (!h) {
    return NULL;
  }

  h->len = 0;
  h->cap = alloc_cap;
  return (float *)(h + 1);
}

/* Returns the number of elements in v, or 0 if v is NULL. */
MSH_INLINE size_t msvec_len(const float *v) {
  return v ? __msvec_hdr(v)->len : 0;
}

/* Returns the allocated element capacity of v, or 0 if v is NULL. */
MSH_INLINE size_t msvec_cap(const float *v) {
  return v ? __msvec_hdr(v)->cap : 0;
}

/* Returns 1 if v is NULL or has 0 elements, 0 otherwise. */
MSH_INLINE int msvec_empty(const float *v) {
  return !v || __msvec_hdr(v)->len == 0;
}

/* Resets the length of v to 0 without freeing its buffer. */
MSH_INLINE void msvec_clear(float *v) {
  if (!v) {
    return;
  }
  __msvec_hdr(v)->len = 0;
}

/*
 * Appends value to v, growing the backing buffer if needed.
 * Passing v == NULL allocates a new vector.
 * Returns the (possibly reallocated) vector pointer, or NULL on OOM
 * (leaving the original pointer valid on failure).
 */
MSH_INLINE float *msvec_push(float *v, float value) {
  if (!v) {
    v = msvec_new(4);
    if (!v) {
      return NULL;
    }
  }

  vec_header *h = __msvec_hdr(v);
  if (h->len >= h->cap) {
    size_t new_cap = h->cap > 0 ? h->cap * 2 : 4;
    if (new_cap < h->cap ||
        new_cap > (SIZE_MAX - sizeof(vec_header)) / sizeof(float)) {
      return NULL;
    }

    vec_header *new_h =
        (vec_header *)realloc(h, sizeof(vec_header) + sizeof(float) * new_cap);
    if (!new_h) {
      return NULL;
    }
    h = new_h;
    h->cap = new_cap;
    v = (float *)(h + 1);
  }

  v[h->len++] = value;
  return v;
}

/* Removes and returns the last element of v, or 0.0f if v is NULL or empty. */
MSH_INLINE float msvec_pop(float *v) {
  if (!v) {
    return 0.0f;
  }
  vec_header *h = __msvec_hdr(v);
  if (h->len == 0) {
    return 0.0f;
  }

  float popped = v[--h->len];
  v[h->len] = 0.0f;
  return popped;
}

/*
 * Computes element-wise sum: result[i] = v[i] + u[i].
 * v and u must have equal length, and result must have capacity >= v's length.
 * Sets result's length to v's length. Returns 1 on success, 0 on error.
 */
MSH_INLINE int msvec_add(const float *v, const float *u, float *result) {
  if (!v || !u || !result) {
    return 0;
  }
  vec_header *vh = __msvec_hdr(v);
  vec_header *uh = __msvec_hdr(u);
  vec_header *rh = __msvec_hdr(result);

  if (vh->len != uh->len || rh->cap < vh->len) {
    return 0;
  }

  for (size_t i = 0; i < vh->len; i++) {
    result[i] = v[i] + u[i];
  }
  rh->len = vh->len;
  return 1;
}

/* Computes the dot product of v and u. Returns 0.0f on NULL or length mismatch.
 */
MSH_INLINE float msvec_mul(const float *v, const float *u) {
  if (!v || !u) {
    return 0.0f;
  }
  vec_header *vh = __msvec_hdr(v);
  vec_header *uh = __msvec_hdr(u);

  if (vh->len != uh->len) {
    return 0.0f;
  }

  float total = 0.0f;
  for (size_t i = 0; i < vh->len; i++) {
    total += v[i] * u[i];
  }
  return total;
}

/* Frees the vector buffer (safe to call with NULL). */
MSH_INLINE void msvec_destroy(float *v) {
  if (!v) {
    return;
  }
  free(__msvec_hdr(v));
}

/* Frees the vector and resets the pointer to NULL. */
#define msvec_free(v)                                                          \
  do {                                                                         \
    msvec_destroy(v);                                                          \
    (v) = NULL;                                                                \
  } while (0)

#ifdef __cplusplus
}
#endif

#endif /* MSVECTOR_H */
