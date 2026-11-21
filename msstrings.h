/*
 * Author: neurd
 * Date: 20th November 2026
 * msstrings.h - v0.0.1: A single header library for manipulating easily your
 * strings!
 *
 * This implementation uses stretchy buffers
 */

#ifndef MSSTRINGS_H
#define MSSTRINGS_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef MSH_INLINE
#if defined(__cplusplus)
#define MSH_INLINE inline
#elif defined(__GNUC__) || defined(__clang__)
#define MSH_INLINE static inline __attribute__((unused))
#else
#define MSH_INLINE static inline
#endif
#endif

typedef struct {
  size_t len;
  size_t cap;
} strviewhdr;

/* Helpers */

MSH_INLINE size_t __msstr_len(char *str);
MSH_INLINE char *__msstr_grow(char *str, size_t capacity);
MSH_INLINE void __msstr_copy(char *dst, const char *src, size_t n);
MSH_INLINE int __msstr_eq(const char *str1, const char *str2, size_t n);

/* API */

MSH_INLINE char *msstr_from_len(char *str, size_t len);
MSH_INLINE char *msstr_new(char *str);
MSH_INLINE int msstr_eq(const char *str1, const char *str2);
MSH_INLINE void msstr_copy(char *msdst, const char *mssrc);
MSH_INLINE void msstr_concat(char *into, char *that);
MSH_INLINE void msstr_split(char *msstr, char *into[], const size_t n_splits,
                            char *sep);
MSH_INLINE void msstr_destroy(char *msstr);

#endif // MSSTRINGS_H

#ifndef MSSTRINGS_IMPLEMENTATION

#ifdef __cplusplus
extern "C" {
#endif

#define hdr(a) ((char *)(a) - sizeof(strviewhdr))

MSH_INLINE size_t __msstr_len(char *str) {
  char *c = str;
  size_t len = 0;
  while (*c != '\0') {
    len++;
    c = str + len;
  }
  return len;
}

MSH_INLINE char *__msstr_grow(char *str, size_t capacity) {
  strviewhdr *h = (strviewhdr *)hdr(str);
  capacity++; // Null terminator

  if (h->cap >= capacity) {
    return NULL;
  }

  void *tmp = realloc(h, sizeof(strviewhdr) + sizeof(char) * capacity);
  if (!tmp) {
    return NULL;
  }

  h = (strviewhdr *)tmp;

  return (char *)(h + 1);
}

MSH_INLINE void __msstr_copy(char *dst, const char *src, size_t n) {
  for (size_t i = 0; i < n; i++) {
    dst[i] = src[i];
  }

  dst[n] = '\0'; // Null terminator
}

MSH_INLINE int __msstr_eq(const char *str1, const char *str2, size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (str1[i] != str2[i]) {
      if (str1[i] > str2[i]) {
        return (int)((uint8_t)(str1[i]) - (uint8_t)(str2[i]));
      } else {
        return (int)((uint8_t)(str2[i]) - (uint8_t)(str1[i]));
      }
    }
  }

  return 0;
}

MSH_INLINE int msstr_eq(const char *str1, const char *str2) {
  strviewhdr *h1 = (strviewhdr *)hdr(str1);
  strviewhdr *h2 = (strviewhdr *)hdr(str2);

  if (h1->len != h2->len) {
    return -1;
  }

  return __msstr_eq(str1, str2, h1->len);
}

MSH_INLINE void msstr_copy(char *msdst, const char *mssrc) {
  strviewhdr *dh = (strviewhdr *)hdr(msdst);
  strviewhdr *sh = (strviewhdr *)hdr(mssrc);

  if (sh->len >= dh->cap)
    return;

  __msstr_copy(msdst, mssrc, sh->len);
  dh->len = sh->len;
}

MSH_INLINE size_t msstr_len(const char *msstr) {
  strviewhdr *h = (strviewhdr *)hdr(msstr);

  return h->len;
}

MSH_INLINE char *msstr_from_len(char *str, size_t len) {
  strviewhdr *h =
      (strviewhdr *)malloc(sizeof(strviewhdr) + sizeof(char) * (len + 1));

  if (!h)
    return NULL;

  h->len = len;
  h->cap = len + 1;

  char *msstr = (char *)(h + 1);
  __msstr_copy(msstr, str, len);

  msstr[h->len] = '\0';

  return msstr;
}

MSH_INLINE char *msstr_new(char *str) {
  return msstr_from_len(str, __msstr_len(str));
}

MSH_INLINE void msstr_concat(char *into, char *that) {
  const strviewhdr *th = (strviewhdr *)hdr(that);
  strviewhdr *ih = (strviewhdr *)hdr(into);

  if (th->len + ih->len >= ih->cap) {
    if (__msstr_grow(into, ih->len + th->len + 1) != 0)
      return;
  }

  for (size_t i = 0; i < th->len; i++) {
    into[ih->len + i] = that[i];
  }

  ih->len += th->len;

  into[ih->len] = '\0';
}

MSH_INLINE void msstr_split(char *msstr, char **into, const size_t n_splits,
                            char *sep) {
  if (sep == NULL || sep[0] == '\0')
    return;

  if (n_splits <= 0)
    return;
  strviewhdr *h = (strviewhdr *)hdr(msstr);
  if (h->len == 0 || h->cap == 0)
    return;

  size_t seplen = __msstr_len(sep);

  if (h->len < seplen)
    return;

  char *ptr = msstr;
  size_t n = 0;
  size_t last_split_pos = 0;

  for (size_t i = 0; i < (h->len - seplen); i++) {
    if (n >= n_splits)
      break;
    if (__msstr_eq(ptr, sep, seplen) == 0) {
      into[n] = msstr_from_len(msstr + last_split_pos, i - last_split_pos);
      last_split_pos = i + seplen;
      i += seplen - 1;
      n++;
    }
    ptr = msstr + i;
  }
}

MSH_INLINE void msstr_destroy(char *msstr) {
  strviewhdr *h = (strviewhdr *)hdr(msstr);
  free(h);
  msstr = NULL;
}

#ifdef __cplusplus
};
#endif

#endif
