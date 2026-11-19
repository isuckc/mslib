/*
 * Author: Neurd
 * Date: 19th November 2026
 *
 * MSVector: A simple single header library for using vectors in your utilities
 * This implementation uses stretchy buffers
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifndef MSVECTOR_H
#define MSVECTOR_H

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

#define hdr(a) ((vec_header *)((char *)(a) - sizeof(vec_header)))

MSH_INLINE float *msvec_new(size_t cap) {
  vec_header *h =
      (vec_header *)malloc(sizeof(vec_header) + sizeof(float) * cap);

  if (h == NULL) {
    return NULL;
  }

  h->len = 0;
  h->cap = cap;

  return (float *)(h + 1);
}

MSH_INLINE int msvec_push(float *v, float value) {
  vec_header *h = hdr(v);
  if (h->len >= h->cap) {
    float *new_vec = (float *)malloc(sizeof(float) * h->cap * 2);
    if (new_vec == NULL)
      return -1;
    memcpy(new_vec, v, h->len);
    h->cap *= 2;
    v = new_vec;
  }

  v[h->len] = value;
  h->len++;

  return -1;
}

MSH_INLINE float msvec_pop(float *v) {
  vec_header *h = hdr(v);

  float popped_value = v[h->len];
  v[h->len] = 0.0f;
  h->len--;

  return popped_value;
}

MSH_INLINE void msvec_add(const float *v, const float *u, float *result) {
  vec_header *vh = hdr(v);
  vec_header *uh = hdr(u);
  vec_header *result_h = hdr(result);

  if (vh->len != uh->len && result_h->len != vh->len) {
    return;
  }

  for (size_t i = 0; i < vh->len; i++) {
    result[i] = v[i] + u[i];
  }
}

MSH_INLINE float msvec_mul(float *v, float *u) {
  vec_header *vh = hdr(v);
  vec_header *uh = hdr(u);

  if (vh->len != uh->len) {
    return 0.0f;
  }

  float total = 0.0f;
  for (size_t i = 0; i < vh->len; i++) {
    total += v[i] * u[i];
  }

  return total;
}

MSH_INLINE void msvec_free(float *v) {
  vec_header *vh = hdr(v);

  free(vh);
}

#ifdef __cplusplus
}
#endif

#endif // MSVECTOR_H
